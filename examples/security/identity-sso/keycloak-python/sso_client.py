#!/usr/bin/env python3
"""
TESAIoT SSO CLI Client

Command-line interface for TESAIoT Platform authentication.

Usage:
    python sso_client.py login          # Interactive login
    python sso_client.py login --device-code  # Device code flow
    python sso_client.py token-info     # Show token info
    python sso_client.py api /devices   # Make API call
    python sso_client.py logout         # Logout

Author: TESAIoT Team
License: Apache 2.0
"""

import asyncio
import json
import sys

import typer
from rich import print as rprint
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich.progress import Progress, SpinnerColumn, TextColumn
import httpx

from keycloak_auth import KeycloakAuth
from config import get_settings
from token_storage import get_token_storage


app = typer.Typer(
    name="tesaiot-sso",
    help="TESAIoT Platform SSO CLI Client",
    add_completion=False,
)
console = Console()


def run_async(coro):
    """Helper to run async functions."""
    return asyncio.get_event_loop().run_until_complete(coro)


@app.command()
def login(
    device_code: bool = typer.Option(
        False, "--device-code", "-d", help="Use device code flow (for headless environments)"
    ),
    service: bool = typer.Option(
        False, "--service", "-s", help="Use client credentials flow (service account)"
    ),
):
    """
    Login to TESAIoT Platform using SSO.

    By default, opens a browser for interactive login.
    Use --device-code for headless environments.
    Use --service for service account authentication.
    """
    auth = KeycloakAuth()

    async def do_login():
        try:
            if service:
                # Client credentials flow
                rprint("\n[blue]🔑 Authenticating as service account...[/blue]")
                tokens = await auth.login_client_credentials()
                rprint("[green]✓ Service authentication successful[/green]")

            elif device_code:
                # Device code flow
                rprint("\n[blue]📱 Starting device code flow...[/blue]")
                dc = await auth.start_device_code_flow()

                rprint(Panel(
                    f"[bold]Go to:[/bold] {dc.verification_uri}\n\n"
                    f"[bold]Enter code:[/bold] [yellow]{dc.user_code}[/yellow]",
                    title="Device Authentication",
                    border_style="blue",
                ))

                with Progress(
                    SpinnerColumn(),
                    TextColumn("[progress.description]{task.description}"),
                    console=console,
                ) as progress:
                    task = progress.add_task("Waiting for authentication...", total=None)

                    def update_status(msg):
                        progress.update(task, description=msg)

                    tokens = await auth.poll_device_code(dc, update_status)

                rprint("[green]✓ Device authentication successful[/green]")

            else:
                # Interactive browser login
                rprint("\n[blue]🌐 Starting browser login...[/blue]")
                rprint("[dim]A browser window will open. Please complete the login.[/dim]")

                tokens = await auth.login_interactive()
                rprint("[green]✓ Login successful[/green]")

            # Show user info
            user = await auth.get_user_info()
            if user:
                rprint(f"\n[bold]Welcome, {user.name}![/bold]")
                rprint(f"Email: {user.email}")
                if user.organization_name:
                    rprint(f"Organization: {user.organization_name}")

        except Exception as e:
            rprint(f"[red]✗ Login failed: {e}[/red]")
            sys.exit(1)
        finally:
            await auth.close()

    run_async(do_login())


@app.command("token-info")
def token_info():
    """
    Display current token information.

    Shows access token claims, expiration, and user roles.
    """
    storage = get_token_storage()

    async def show_info():
        tokens = await storage.load()
        if not tokens:
            rprint("[yellow]No tokens found. Please login first.[/yellow]")
            sys.exit(1)

        # Token status
        table = Table(title="Token Status")
        table.add_column("Property", style="cyan")
        table.add_column("Value", style="green")

        table.add_row(
            "Access Token Valid",
            "✓ Yes" if not tokens.is_access_token_expired() else "✗ Expired"
        )
        table.add_row(
            "Access Token Expires",
            tokens.access_token_expires_at.strftime("%Y-%m-%d %H:%M:%S UTC")
            if tokens.access_token_expires_at else "Unknown"
        )
        table.add_row(
            "Refresh Token Valid",
            "✓ Yes" if tokens.refresh_token and not tokens.is_refresh_token_expired() else "✗ Expired"
        )

        rprint(table)

        # User info
        auth = KeycloakAuth(token_storage=storage)
        user = await auth.get_user_info()
        await auth.close()

        if user:
            rprint("\n[bold]User Information[/bold]")
            user_table = Table(show_header=False)
            user_table.add_column("Property", style="cyan")
            user_table.add_column("Value")

            user_table.add_row("User ID", user.sub)
            user_table.add_row("Username", user.preferred_username or "N/A")
            user_table.add_row("Name", user.name or "N/A")
            user_table.add_row("Email", user.email or "N/A")
            user_table.add_row("Email Verified", "✓" if user.email_verified else "✗")
            user_table.add_row("Organization", user.organization_name or "N/A")
            user_table.add_row("Roles", ", ".join(user.roles) if user.roles else "None")

            rprint(user_table)

    run_async(show_info())


@app.command()
def api(
    endpoint: str = typer.Argument(..., help="API endpoint (e.g., /devices)"),
    method: str = typer.Option("GET", "--method", "-X", help="HTTP method"),
    data: str = typer.Option(None, "--data", "-d", help="Request body (JSON)"),
):
    """
    Make an authenticated API call to TESAIoT.

    Example:
        python sso_client.py api /devices
        python sso_client.py api /devices -X POST -d '{"name": "test"}'
    """
    settings = get_settings()
    auth = KeycloakAuth()

    async def call_api():
        try:
            token = await auth.get_valid_token()
            if not token:
                rprint("[yellow]No valid token. Please login first.[/yellow]")
                sys.exit(1)

            url = f"{settings.tesaiot_api_url}{endpoint}"
            headers = {
                "Authorization": f"Bearer {token}",
                "Content-Type": "application/json",
            }

            async with httpx.AsyncClient() as client:
                if method.upper() == "GET":
                    response = await client.get(url, headers=headers)
                elif method.upper() == "POST":
                    response = await client.post(
                        url, headers=headers, json=json.loads(data) if data else None
                    )
                elif method.upper() == "PUT":
                    response = await client.put(
                        url, headers=headers, json=json.loads(data) if data else None
                    )
                elif method.upper() == "DELETE":
                    response = await client.delete(url, headers=headers)
                else:
                    rprint(f"[red]Unsupported method: {method}[/red]")
                    sys.exit(1)

            # Show response
            rprint(f"\n[bold]Status:[/bold] {response.status_code}")
            try:
                result = response.json()
                rprint(f"[bold]Response:[/bold]")
                rprint(json.dumps(result, indent=2))
            except Exception:
                rprint(f"[bold]Response:[/bold] {response.text}")

        except Exception as e:
            rprint(f"[red]✗ API call failed: {e}[/red]")
            sys.exit(1)
        finally:
            await auth.close()

    run_async(call_api())


@app.command()
def refresh():
    """
    Refresh the access token.

    Uses the stored refresh token to get a new access token.
    """
    auth = KeycloakAuth()

    async def do_refresh():
        try:
            tokens = await auth.refresh_token()
            rprint("[green]✓ Token refreshed successfully[/green]")
            rprint(f"New token expires: {tokens.access_token_expires_at}")
        except Exception as e:
            rprint(f"[red]✗ Token refresh failed: {e}[/red]")
            sys.exit(1)
        finally:
            await auth.close()

    run_async(do_refresh())


@app.command()
def logout():
    """
    Logout and clear stored tokens.

    Revokes the refresh token and clears local storage.
    """
    auth = KeycloakAuth()

    async def do_logout():
        try:
            await auth.logout()
            rprint("[green]✓ Logged out successfully[/green]")
        except Exception as e:
            rprint(f"[yellow]Warning: {e}[/yellow]")
        finally:
            await auth.close()

    run_async(do_logout())


@app.command()
def config():
    """
    Show current configuration.

    Displays Keycloak endpoints and settings.
    """
    settings = get_settings()

    table = Table(title="TESAIoT SSO Configuration")
    table.add_column("Setting", style="cyan")
    table.add_column("Value", style="green")

    table.add_row("Keycloak URL", settings.keycloak_url)
    table.add_row("Realm", settings.keycloak_realm)
    table.add_row("Client ID", settings.keycloak_client_id)
    table.add_row("Client Secret", "****" if settings.keycloak_client_secret else "Not Set")
    table.add_row("Token URL", settings.token_url)
    table.add_row("API URL", settings.tesaiot_api_url)
    table.add_row("Token Storage", settings.token_storage)

    rprint(table)


if __name__ == "__main__":
    app()

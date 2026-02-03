import React from "react";
import ReactDOM from "react-dom/client";
import { BrowserRouter } from "react-router-dom";
import { ReactKeycloakProvider } from "@react-keycloak/web";
import App from "./App";
import keycloak from "./services/keycloak";
import "./index.css";

/**
 * Keycloak initialization options
 * - onLoad: 'check-sso' checks for existing session without forcing login
 * - silentCheckSsoRedirectUri: enables silent token refresh via iframe
 * - pkceMethod: 'S256' for secure PKCE flow
 */
const initOptions = {
  onLoad: "check-sso" as const,
  silentCheckSsoRedirectUri: `${window.location.origin}/silent-check-sso.html`,
  pkceMethod: "S256" as const,
};

/**
 * Event handler for Keycloak events
 */
const eventHandler = (event: string, error?: Error) => {
  if (error) {
    console.error("Keycloak event error:", event, error);
  } else {
    console.log("Keycloak event:", event);
  }
};

/**
 * Token handler - called when tokens are refreshed
 */
const tokenHandler = (tokens: { token?: string }) => {
  if (tokens.token) {
    console.log("Token refreshed");
  }
};

ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <ReactKeycloakProvider
      authClient={keycloak}
      initOptions={initOptions}
      onEvent={eventHandler}
      onTokens={tokenHandler}>
      <BrowserRouter>
        <App />
      </BrowserRouter>
    </ReactKeycloakProvider>
  </React.StrictMode>,
);

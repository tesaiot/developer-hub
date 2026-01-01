/**
 * Complete Dashboard Example for TESAIoT Analytics API (JavaScript)
 *
 * This example demonstrates building a comprehensive IoT analytics
 * dashboard that combines all four analytics domains.
 *
 * Author: TESAIoT Team
 * License: Apache 2.0
 */

import { AnalyticsClient, TimeRange } from './analytics-client.js';

/**
 * Collect all dashboard data in parallel
 */
async function collectDashboardData(client) {
    const [
        anomalies,
        timeline,
        clusters,
        insights,
        connectivity,
        latency,
        throughput,
        quality
    ] = await Promise.all([
        client.getAnomalies({ limit: 100, severityFilter: ['critical', 'high', 'medium'] }),
        client.getAnomalyTimeline({ days: 30 }),
        client.getClusters({ metricName: 'temperature', nClusters: 5 }),
        client.getInsights({ days: 7 }),
        client.getConnectivityStatus(),
        client.getLatencyStats({ hours: 24 }),
        client.getThroughputStats({ hours: 24 }),
        client.getConnectionQuality()
    ]);

    const alerts = generateAlerts(anomalies, connectivity, latency, quality);
    const fleetHealth = calculateFleetHealth(anomalies, connectivity, insights, latency);

    return {
        timestamp: new Date().toISOString(),
        fleetHealth,
        anomalies: {
            summary: anomalies.summary || {},
            recent: (anomalies.anomalies || []).slice(0, 10),
            timeline: timeline.timeline || [],
            trend: timeline.trend || {}
        },
        clusters: {
            clusters: clusters.clusters || [],
            silhouetteScore: clusters.silhouette_score || 0,
            outliers: (clusters.outliers || []).slice(0, 5)
        },
        insights: insights.insights || [],
        connectivity: {
            status: connectivity.summary || {},
            devices: (connectivity.devices || []).slice(0, 20),
            latency: latency.summary || {},
            throughput: throughput.summary || {},
            quality: quality.summary || {}
        },
        alerts
    };
}

/**
 * Calculate overall fleet health score
 */
function calculateFleetHealth(anomalies, connectivity, insights, latency) {
    const scores = {};

    // Anomaly score
    const anomalySummary = anomalies.summary || {};
    const totalDevices = connectivity.summary?.total || 100;
    const anomalyCount = anomalySummary.total || 0;
    const anomalyRate = anomalyCount / Math.max(totalDevices, 1);
    scores.anomaly = Math.max(0, 100 - (anomalyRate * 1000));

    // Connectivity score
    const connSummary = connectivity.summary || {};
    const online = connSummary.online || 0;
    const total = connSummary.total || 1;
    scores.connectivity = (online / Math.max(total, 1)) * 100;

    // Latency score
    const p95Latency = latency.summary?.p95_latency_ms || 0;
    scores.latency = Math.max(0, 100 - (p95Latency / 10));

    // Insights severity score
    const insightsList = insights.insights || [];
    const criticalCount = insightsList.filter(i => i.severity === 'critical').length;
    const warningCount = insightsList.filter(i => i.severity === 'warning').length;
    scores.insights = Math.max(0, 100 - (criticalCount * 20) - (warningCount * 5));

    // Calculate weighted overall score
    const overall =
        scores.anomaly * 0.3 +
        scores.connectivity * 0.3 +
        scores.latency * 0.2 +
        scores.insights * 0.2;

    return {
        overallScore: Math.round(overall * 10) / 10,
        componentScores: scores,
        status: getHealthStatus(overall)
    };
}

/**
 * Convert health score to status label
 */
function getHealthStatus(score) {
    if (score >= 90) return 'EXCELLENT';
    if (score >= 70) return 'GOOD';
    if (score >= 50) return 'FAIR';
    if (score >= 30) return 'POOR';
    return 'CRITICAL';
}

/**
 * Generate alerts based on current data
 */
function generateAlerts(anomalies, connectivity, latency, quality) {
    const alerts = [];

    // Critical anomaly alerts
    const criticalCount = anomalies.summary?.by_severity?.critical || 0;
    if (criticalCount > 0) {
        alerts.push({
            level: 'critical',
            type: 'anomaly',
            title: `${criticalCount} Critical Anomalies Detected`,
            description: 'Immediate investigation recommended'
        });
    }

    // Offline device alerts
    const offline = connectivity.summary?.offline || 0;
    const total = connectivity.summary?.total || 1;
    const offlinePct = (offline / Math.max(total, 1)) * 100;

    if (offlinePct > 20) {
        alerts.push({
            level: 'critical',
            type: 'connectivity',
            title: `${offline} Devices Offline (${offlinePct.toFixed(0)}%)`,
            description: 'Network connectivity issue detected'
        });
    } else if (offline > 0) {
        alerts.push({
            level: 'warning',
            type: 'connectivity',
            title: `${offline} Device(s) Offline`,
            description: 'Some devices are not responding'
        });
    }

    // Latency alerts
    const p95 = latency.summary?.p95_latency_ms || 0;
    if (p95 > 1000) {
        alerts.push({
            level: 'critical',
            type: 'latency',
            title: `High Latency: ${p95.toFixed(0)}ms P95`,
            description: 'Network performance severely degraded'
        });
    } else if (p95 > 500) {
        alerts.push({
            level: 'warning',
            type: 'latency',
            title: `Elevated Latency: ${p95.toFixed(0)}ms P95`,
            description: 'Network performance degraded'
        });
    }

    // Quality alerts
    const poorDevices = quality.summary?.distribution?.poor || 0;
    if (poorDevices > 5) {
        alerts.push({
            level: 'warning',
            type: 'quality',
            title: `${poorDevices} Devices with Poor Connection Quality`,
            description: 'Review device connections and network path'
        });
    }

    return alerts;
}

/**
 * Render dashboard to console
 */
function renderDashboard(data) {
    console.log('\n' + '='.repeat(80));
    console.log(' '.repeat(25) + 'TESAIoT ANALYTICS DASHBOARD');
    console.log('='.repeat(80));
    console.log(`Generated: ${data.timestamp}`);

    // Fleet Health
    console.log('\n' + '-'.repeat(80));
    console.log(' FLEET HEALTH');
    console.log('-'.repeat(80));

    const { overallScore, status, componentScores } = data.fleetHealth;
    const barLen = Math.floor(overallScore / 5);
    const bar = '\u2588'.repeat(barLen) + '\u2591'.repeat(20 - barLen);
    console.log(`\n  Overall: [${bar}] ${overallScore}/100 (${status})`);

    console.log('\n  Component Scores:');
    for (const [component, score] of Object.entries(componentScores)) {
        console.log(`    ${component.padEnd(15)} ${Math.round(score)}/100`);
    }

    // Alerts
    if (data.alerts.length > 0) {
        console.log('\n' + '-'.repeat(80));
        console.log(' ACTIVE ALERTS');
        console.log('-'.repeat(80));

        for (const alert of data.alerts) {
            const icon = alert.level === 'critical' ? '\u2757' : '\u26A0';
            console.log(`\n  [${icon}] ${alert.title}`);
            console.log(`      ${alert.description}`);
        }
    }

    // Anomalies
    console.log('\n' + '-'.repeat(80));
    console.log(' ANOMALY DETECTION');
    console.log('-'.repeat(80));

    const anomSummary = data.anomalies.summary;
    console.log(`\n  Total Anomalies (7 days): ${anomSummary.total || 0}`);

    if (anomSummary.by_severity) {
        console.log('  By Severity:');
        for (const [sev, count] of Object.entries(anomSummary.by_severity)) {
            console.log(`    ${sev.padEnd(10)} ${count}`);
        }
    }

    const trend = data.anomalies.trend;
    const trendIcon = trend.direction === 'decreasing' ? '\u2193' :
                      trend.direction === 'increasing' ? '\u2191' : '\u2194';
    console.log(`\n  Trend: ${trendIcon} ${trend.direction || 'stable'} (${(trend.percent_change || 0).toFixed(1)}%)`);

    // Clusters
    console.log('\n' + '-'.repeat(80));
    console.log(' PATTERN RECOGNITION');
    console.log('-'.repeat(80));

    console.log(`\n  Clusters: ${data.clusters.clusters.length}`);
    console.log(`  Silhouette Score: ${(data.clusters.silhouetteScore || 0).toFixed(3)}`);

    for (const cluster of data.clusters.clusters) {
        console.log(`\n    Cluster ${cluster.cluster_id}: ${cluster.device_count} devices`);
    }

    if (data.clusters.outliers.length > 0) {
        console.log(`\n  Outliers: ${data.clusters.outliers.length} devices`);
    }

    // Insights
    console.log('\n' + '-'.repeat(80));
    console.log(' AI INSIGHTS');
    console.log('-'.repeat(80));

    console.log(`\n  Total Insights: ${data.insights.length}`);

    const criticalInsights = data.insights.filter(i => i.severity === 'critical');
    if (criticalInsights.length > 0) {
        console.log('\n  Critical Insights:');
        for (const i of criticalInsights.slice(0, 3)) {
            console.log(`    \u2757 ${i.title}`);
        }
    }

    const actionable = data.insights.filter(i => i.actionable);
    if (actionable.length > 0) {
        console.log(`\n  Actionable Recommendations (${actionable.length}):`);
        for (const i of actionable.slice(0, 3)) {
            console.log(`    - ${i.title}`);
        }
    }

    // Connectivity
    console.log('\n' + '-'.repeat(80));
    console.log(' CONNECTIVITY');
    console.log('-'.repeat(80));

    const connStatus = data.connectivity.status;
    console.log(`\n  Devices: ${connStatus.online || 0}/${connStatus.total || 0} online`);

    const latencyStats = data.connectivity.latency;
    console.log('\n  Latency:');
    console.log(`    Average: ${(latencyStats.avg_latency_ms || 0).toFixed(1)} ms`);
    console.log(`    P95: ${(latencyStats.p95_latency_ms || 0).toFixed(1)} ms`);

    const throughputStats = data.connectivity.throughput;
    console.log('\n  Throughput (24h):');
    console.log(`    Total Messages: ${(throughputStats.total_messages || 0).toLocaleString()}`);
    console.log(`    Avg/Hour: ${(throughputStats.avg_per_hour || 0).toLocaleString()}`);

    const qualityStats = data.connectivity.quality;
    console.log(`\n  Connection Quality: ${(qualityStats.average_quality_score || 0).toFixed(0)}/100`);

    // Footer
    console.log('\n' + '='.repeat(80));
    console.log(' '.repeat(30) + 'END OF REPORT');
    console.log('='.repeat(80));
}

/**
 * Dashboard refresh loop
 */
async function dashboardRefreshLoop(client, intervalSeconds = 60, maxIterations = 5) {
    console.log('Starting dashboard refresh loop...');
    console.log(`Refresh interval: ${intervalSeconds} seconds`);
    console.log('Press Ctrl+C to stop\n');

    for (let i = 0; i < maxIterations; i++) {
        try {
            const data = await collectDashboardData(client);
            console.log('\n'.repeat(2));
            renderDashboard(data);
            console.log(`\n[Refresh ${i + 1}/${maxIterations} - Next refresh in ${intervalSeconds} seconds]`);
            await new Promise(resolve => setTimeout(resolve, intervalSeconds * 1000));
        } catch (error) {
            console.error(`Error refreshing dashboard: ${error.message}`);
            await new Promise(resolve => setTimeout(resolve, 10000));
        }
    }

    console.log('\nDashboard loop completed.');
}

/**
 * Export dashboard data to JSON
 */
async function exportDashboardJson(client, outputFile = 'dashboard_data.json') {
    console.log(`Exporting dashboard data to ${outputFile}...`);
    const data = await collectDashboardData(client);

    const fs = await import('fs');
    fs.writeFileSync(outputFile, JSON.stringify(data, null, 2));

    console.log(`Dashboard data exported to ${outputFile}`);
}

// Main entry point
async function main() {
    const client = new AnalyticsClient();
    const args = process.argv.slice(2);

    if (args.includes('--loop')) {
        const interval = parseInt(args[args.indexOf('--loop') + 1]) || 60;
        await dashboardRefreshLoop(client, interval);
    } else if (args.includes('--export')) {
        const output = args[args.indexOf('--export') + 1] || 'dashboard_data.json';
        await exportDashboardJson(client, output);
    } else if (args.includes('--help')) {
        console.log('TESAIoT Analytics Dashboard');
        console.log('\nUsage:');
        console.log('  node example-dashboard.js          # Single refresh');
        console.log('  node example-dashboard.js --loop   # Continuous refresh (60s)');
        console.log('  node example-dashboard.js --loop 30  # Custom interval');
        console.log('  node example-dashboard.js --export  # Export to JSON');
    } else {
        console.log('Collecting dashboard data...');
        const data = await collectDashboardData(client);
        renderDashboard(data);
    }
}

main().catch(console.error);

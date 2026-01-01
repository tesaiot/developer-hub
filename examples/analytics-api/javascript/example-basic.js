/**
 * Basic Usage Example for TESAIoT Analytics API (JavaScript)
 *
 * This example demonstrates:
 * - Client initialization
 * - Making API requests
 * - Handling responses
 * - Error handling
 *
 * Author: TESAIoT Team
 * License: Apache 2.0
 */

import { AnalyticsClient, TimeRange } from './analytics-client.js';

async function basicExample() {
    console.log('='.repeat(60));
    console.log('TESAIoT Analytics API - Basic Example (JavaScript)');
    console.log('='.repeat(60));

    const client = new AnalyticsClient({
        baseUrl: process.env.TESAIOT_API_URL || 'https://admin.tesaiot.com/api/v1/bdh-ai',
        apiToken: process.env.TESAIOT_API_TOKEN
    });

    // 1. Get recent anomalies
    console.log('\n1. Fetching recent anomalies...');
    try {
        const anomalies = await client.getAnomalies({
            limit: 5
            // Note: severity values are 'critical', 'medium', 'low'
        });
        console.log(`   Found ${anomalies.anomalies?.length || 0} anomalies`);

        for (const a of (anomalies.anomalies || []).slice(0, 3)) {
            console.log(`   - ${a.device_name}: ${a.metric} (${a.severity})`);
        }
    } catch (e) {
        console.log(`   Error: ${e.message}`);
    }

    // 2. Get latency statistics
    console.log('\n2. Fetching latency statistics...');
    try {
        const latency = await client.getLatencyStats({ hours: 24 });
        const stats = latency.summary || latency;
        console.log(`   Average latency: ${stats.average_ms || stats.avg_latency_ms || 'N/A'} ms`);
        console.log(`   P95 latency: ${stats.p95_ms || stats.p95_latency_ms || 'N/A'} ms`);
    } catch (e) {
        console.log(`   Error: ${e.message}`);
    }

    // 3. Get throughput statistics
    console.log('\n3. Fetching throughput statistics...');
    try {
        const throughput = await client.getThroughputStats({ hours: 24 });
        const stats = throughput.summary || throughput;
        console.log(`   Total messages: ${stats.total_messages || 'N/A'}`);
        console.log(`   Messages per hour: ${stats.messages_per_hour || 'N/A'}`);
    } catch (e) {
        console.log(`   Error: ${e.message}`);
    }

    // 4. Get connectivity status
    console.log('\n4. Fetching connectivity status...');
    try {
        const status = await client.getConnectivityStatus();
        const summary = status.summary || {};
        console.log(`   Total devices: ${summary.total || 0}`);
        console.log(`   Online: ${summary.online || 0}`);
        console.log(`   Offline: ${summary.offline || 0}`);
    } catch (e) {
        console.log(`   Error: ${e.message}`);
    }

    console.log('\n' + '='.repeat(60));
    console.log('Done!');
}

function timeRangeExamples() {
    console.log('='.repeat(60));
    console.log('Time Range Examples');
    console.log('='.repeat(60));

    // Using TimeRange helper
    const last7Days = TimeRange.lastDays(7);
    console.log('\nLast 7 days:');
    console.log(`  Start: ${last7Days.start}`);
    console.log(`  End: ${last7Days.end}`);

    const last24Hours = TimeRange.lastHours(24);
    console.log('\nLast 24 hours:');
    console.log(`  Start: ${last24Hours.start}`);
    console.log(`  End: ${last24Hours.end}`);

    // Custom time range
    const customStart = new Date('2025-12-01T00:00:00Z');
    const customEnd = new Date('2025-12-29T23:59:59Z');
    console.log('\nCustom range:');
    console.log(`  Start: ${customStart.toISOString()}`);
    console.log(`  End: ${customEnd.toISOString()}`);
}

// Run examples
const args = process.argv.slice(2);

if (args.includes('--time')) {
    timeRangeExamples();
} else {
    basicExample().catch(console.error);
}

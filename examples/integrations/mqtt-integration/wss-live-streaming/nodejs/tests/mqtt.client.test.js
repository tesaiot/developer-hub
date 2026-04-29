/**
 * Tests for TESAIoT WSS Live Streaming Node.js Client
 *
 * Run with: npm test
 */

// Store original env
const originalEnv = process.env;

// Helper to create a mock MQTT client
function createMockClient() {
  return {
    on: jest.fn(),
    subscribe: jest.fn(),
    end: jest.fn((force, callback) => callback && callback()),
  };
}

// Helper to setup test environment
function setupTest(token = "tesa_mqtt_testorg_1234567890123456789012345678") {
  // Reset modules
  jest.resetModules();
  jest.clearAllMocks();

  // Reset environment
  process.env = { ...originalEnv };

  if (token) {
    process.env.MQTT_API_TOKEN = token;
  } else {
    delete process.env.MQTT_API_TOKEN;
  }

  // Create mock client
  const mockClient = createMockClient();

  // Mock mqtt module
  jest.doMock("mqtt", () => ({
    connect: jest.fn(() => mockClient),
  }));

  // Mock dotenv
  jest.doMock("dotenv", () => ({
    config: jest.fn(),
  }));

  return { mockClient };
}

// Helper to load the module
function loadModule() {
  return require("../index.js");
}

describe("Config Validation", () => {
  afterEach(() => {
    jest.dontMock("mqtt");
    jest.dontMock("dotenv");
  });

  afterAll(() => {
    process.env = originalEnv;
  });

  test("should exit when MQTT_API_TOKEN is missing", () => {
    const { mockClient } = setupTest(null);
    const mockExit = jest.spyOn(process, "exit").mockImplementation(() => {});
    const mockError = jest.spyOn(console, "error").mockImplementation(() => {});

    loadModule();

    expect(mockExit).toHaveBeenCalledWith(1);
    expect(mockError).toHaveBeenCalledWith(
      expect.stringContaining("MQTT_API_TOKEN is required")
    );
  });

  test("should exit when token format is invalid", () => {
    const { mockClient } = setupTest("invalid_token_format");
    const mockExit = jest.spyOn(process, "exit").mockImplementation(() => {});
    const mockError = jest.spyOn(console, "error").mockImplementation(() => {});

    loadModule();

    expect(mockExit).toHaveBeenCalledWith(1);
    expect(mockError).toHaveBeenCalledWith(
      expect.stringContaining("Invalid token format")
    );
  });

  test("should accept valid token format", () => {
    const { mockClient } = setupTest("tesa_mqtt_testorg_1234567890123456789012345678");
    const mockExit = jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    // Should not exit with valid token
    expect(mockExit).not.toHaveBeenCalled();
  });

  test("should use default values when environment variables are not set", () => {
    const { mockClient } = setupTest();
    jest.spyOn(process, "exit").mockImplementation(() => {});

    const mqtt = require("mqtt");
    loadModule();

    // Check that mqtt.connect was called with defaults
    expect(mqtt.connect).toHaveBeenCalled();
    const callArgs = mqtt.connect.mock.calls[0];

    expect(callArgs[0]).toBe("wss://mqtt.tesaiot.com:8085/mqtt");
    expect(callArgs[1].clientId).toMatch(/^tesaiot-nodejs-/);
    expect(callArgs[1].reconnectPeriod).toBe(5000);
    expect(callArgs[1].keepalive).toBe(60);
  });

  test("should use environment variable values when set", () => {
    const { mockClient } = setupTest();
    process.env.MQTT_BROKER_URL = "wss://custom.broker.com:9001/mqtt";
    process.env.MQTT_CLIENT_ID = "my-custom-client";
    process.env.MQTT_RECONNECT_PERIOD = "10000";
    process.env.MQTT_KEEPALIVE = "120";

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const mqtt = require("mqtt");
    loadModule();

    const callArgs = mqtt.connect.mock.calls[0];

    expect(callArgs[0]).toBe("wss://custom.broker.com:9001/mqtt");
    expect(callArgs[1].clientId).toBe("my-custom-client");
    expect(callArgs[1].reconnectPeriod).toBe(10000);
    expect(callArgs[1].keepalive).toBe(120);
  });

  test("should set token as both username and password", () => {
    const token = "tesa_mqtt_testorg_1234567890123456789012345678";
    const { mockClient } = setupTest(token);
    jest.spyOn(process, "exit").mockImplementation(() => {});

    const mqtt = require("mqtt");
    loadModule();

    const callArgs = mqtt.connect.mock.calls[0];

    expect(callArgs[1].username).toBe(token);
    expect(callArgs[1].password).toBe(token);
  });
});

describe("Topic Parsing", () => {
  afterEach(() => {
    jest.dontMock("mqtt");
    jest.dontMock("dotenv");
  });

  test("should parse device ID from standard topic format", () => {
    const { mockClient } = setupTest();
    process.env.MQTT_TOPIC = "device/+/telemetry/#";

    jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    // Get the message handler
    const messageHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "message"
    )[1];

    // Mock console.log to capture output
    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    // Simulate message
    const topic = "device/abc-123/telemetry/temperature";
    const payload = Buffer.from(JSON.stringify({ value: 25.5 }));
    messageHandler(topic, payload);

    expect(consoleSpy).toHaveBeenCalledWith(
      expect.stringContaining("abc-123")
    );
  });

  test("should parse sensor type from nested topic", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    const messageHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "message"
    )[1];

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    const topic = "device/my-device/telemetry/humidity/relative";
    const payload = Buffer.from(JSON.stringify({ value: 60 }));
    messageHandler(topic, payload);

    expect(consoleSpy).toHaveBeenCalledWith(
      expect.stringContaining("humidity/relative")
    );
  });

  test("should handle topic with default sensor type", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    const messageHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "message"
    )[1];

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    // Topic without sensor type (only 2 parts)
    const topic = "device/short-topic";
    const payload = Buffer.from(JSON.stringify({ data: "test" }));
    messageHandler(topic, payload);

    expect(consoleSpy).toHaveBeenCalledWith(
      expect.stringContaining("default")
    );
  });

  test("should subscribe to configured topic", () => {
    const { mockClient } = setupTest();
    process.env.MQTT_TOPIC = "device/my-device/telemetry/#";

    jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    // Get the connect handler
    const connectHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "connect"
    )[1];

    connectHandler();

    expect(mockClient.subscribe).toHaveBeenCalledWith(
      "device/my-device/telemetry/#",
      { qos: 1 },
      expect.any(Function)
    );
  });
});

describe("Message Parsing", () => {
  afterEach(() => {
    jest.dontMock("mqtt");
    jest.dontMock("dotenv");
  });

  test("should parse JSON payload correctly", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    const messageHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "message"
    )[1];

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    const topic = "device/test-device/telemetry/temperature";
    const payload = Buffer.from(
      JSON.stringify({ value: 25.5, unit: "celsius", timestamp: "2024-01-01T00:00:00Z" })
    );
    messageHandler(topic, payload);

    // Check that output contains expected values
    const calls = consoleSpy.mock.calls;
    const allOutput = calls.map((c) => c.join(" ")).join(" ");
    expect(allOutput).toContain("25.5");
    expect(allOutput).toContain("test-device");
  });

  test("should handle non-JSON payload gracefully", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    const messageHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "message"
    )[1];

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    const topic = "device/test-device/telemetry/status";
    const payload = Buffer.from("online");
    messageHandler(topic, payload);

    // Check that "online" appears in the console output
    const calls = consoleSpy.mock.calls;
    const allOutput = calls.map((call) => call.join(" ")).join(" ");
    expect(allOutput).toContain("online");
  });

  test("should handle empty payload", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    const messageHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "message"
    )[1];

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    const topic = "device/test-device/telemetry/empty";
    const payload = Buffer.from("");
    messageHandler(topic, payload);

    // Should not throw an error
    expect(consoleSpy).toHaveBeenCalled();
  });

  test("should handle nested JSON payload", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    const messageHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "message"
    )[1];

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    const topic = "device/test-device/telemetry/sensors";
    const payload = Buffer.from(
      JSON.stringify({
        temperature: { value: 25.5, unit: "C" },
        humidity: { value: 60, unit: "%" },
      })
    );
    messageHandler(topic, payload);

    // Check that the output contains temperature and humidity data
    const calls = consoleSpy.mock.calls;
    const allOutput = calls.map(call => call.join(" ")).join(" ");
    expect(allOutput).toContain("temperature");
    expect(allOutput).toContain("humidity");
  });

  test("should include timestamp in output", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    loadModule();

    const messageHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "message"
    )[1];

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    const topic = "device/test-device/telemetry/data";
    const payload = Buffer.from(JSON.stringify({ value: 100 }));
    messageHandler(topic, payload);

    // Check that timestamp is included (ISO 8601 format)
    const logCalls = consoleSpy.mock.calls;
    const timestampCall = logCalls.find((call) =>
      call[0].match(/\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}/)
    );
    expect(timestampCall).toBeDefined();
  });
});

describe("MQTT Client Events", () => {
  afterEach(() => {
    jest.dontMock("mqtt");
    jest.dontMock("dotenv");
  });

  test("should handle connection event", () => {
    const { mockClient } = setupTest();
    mockClient.subscribe = jest.fn((topic, opts, callback) => {
      callback(null, [{ topic, qos: 1 }]);
    });

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    loadModule();

    const connectHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "connect"
    )[1];

    connectHandler();

    expect(consoleSpy).toHaveBeenCalledWith(
      expect.stringContaining("Connected")
    );
  });

  test("should handle subscription error", () => {
    const { mockClient } = setupTest();
    mockClient.subscribe = jest.fn((topic, opts, callback) => {
      callback(new Error("Subscription failed"), null);
    });

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const consoleSpy = jest
      .spyOn(console, "error")
      .mockImplementation(() => {});

    loadModule();

    const connectHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "connect"
    )[1];

    connectHandler();

    // Check that error message contains "Subscription error"
    const calls = consoleSpy.mock.calls;
    const found = calls.some((call) =>
      call.some((arg) =>
        typeof arg === "string" && arg.includes("Subscription error")
      )
    );
    expect(found).toBe(true);
  });

  test("should handle connection error", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const consoleSpy = jest
      .spyOn(console, "error")
      .mockImplementation(() => {});

    loadModule();

    const errorHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "error"
    )[1];

    errorHandler(new Error("Connection refused"));

    // Check that error message contains "Connection error"
    const calls = consoleSpy.mock.calls;
    const found = calls.some(call =>
      call.some(arg =>
        typeof arg === "string" && arg.includes("Connection error")
      )
    );
    expect(found).toBe(true);
  });

  test("should handle disconnect event", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    loadModule();

    const closeHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "close"
    )[1];

    closeHandler();

    expect(consoleSpy).toHaveBeenCalledWith(
      expect.stringContaining("Disconnected")
    );
  });

  test("should handle reconnect event", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    loadModule();

    const reconnectHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "reconnect"
    )[1];

    reconnectHandler();

    expect(consoleSpy).toHaveBeenCalledWith(
      expect.stringContaining("Reconnecting")
    );
  });

  test("should handle offline event", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const consoleSpy = jest.spyOn(console, "log").mockImplementation(() => {});

    loadModule();

    const offlineHandler = mockClient.on.mock.calls.find(
      (call) => call[0] === "offline"
    )[1];

    offlineHandler();

    expect(consoleSpy).toHaveBeenCalledWith(
      expect.stringContaining("offline")
    );
  });
});

describe("Graceful Shutdown", () => {
  afterEach(() => {
    jest.dontMock("mqtt");
    jest.dontMock("dotenv");
  });

  test("should handle SIGINT signal", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const processOnSpy = jest.spyOn(process, "on").mockImplementation(() => {});

    loadModule();

    // Check that SIGINT handler was registered
    const sigintHandler = processOnSpy.mock.calls.find(
      (call) => call[0] === "SIGINT"
    );
    expect(sigintHandler).toBeDefined();
  });

  test("should handle SIGTERM signal", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const processOnSpy = jest.spyOn(process, "on").mockImplementation(() => {});

    loadModule();

    // Check that SIGTERM handler was registered
    const sigtermHandler = processOnSpy.mock.calls.find(
      (call) => call[0] === "SIGTERM"
    );
    expect(sigtermHandler).toBeDefined();
  });
});

describe("Connection Options", () => {
  afterEach(() => {
    jest.dontMock("mqtt");
    jest.dontMock("dotenv");
  });

  test("should set rejectUnauthorized to false by default", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const mqtt = require("mqtt");
    loadModule();

    const callArgs = mqtt.connect.mock.calls[0];
    expect(callArgs[1].rejectUnauthorized).toBe(false);
  });

  test("should set clean session to true", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const mqtt = require("mqtt");
    loadModule();

    const callArgs = mqtt.connect.mock.calls[0];
    expect(callArgs[1].clean).toBe(true);
  });

  test("should set connect timeout", () => {
    const { mockClient } = setupTest();

    jest.spyOn(process, "exit").mockImplementation(() => {});

    const mqtt = require("mqtt");
    loadModule();

    const callArgs = mqtt.connect.mock.calls[0];
    expect(callArgs[1].connectTimeout).toBe(30000);
  });
});

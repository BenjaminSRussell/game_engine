# AEGIS Go Adversarial Server Integration

## Overview

This document details the integration between the AEGIS Dashboard and the Go-based adversarial server. The adversarial server simulates various bot detection mechanisms and defensive countermeasures, providing a realistic testing environment for scraper resilience.

## Go Adversarial Server Parameters

### Server Configuration (config.yaml)

```yaml
# Server Settings
server:
  port: 8080
  host: 0.0.0.0
  read_timeout: 30s
  write_timeout: 30s

# Detection Modules
detection:
  tls:
    enabled: true
    ja3_database: "./fingerprints/ja3.json"
  
  http2:
    enabled: true
    akamai_enabled: true
  
  fingerprinting:
    canvas_enabled: true
    webgl_enabled: true
    audio_enabled: true
    fonts_enabled: true
  
  behavioral:
    mouse_tracking: true
    timing_analysis: true
    min_mouse_events: 3
    max_pow_duration_ms: 5000
  
  automation:
    check_webdriver: true
    check_chrome_headless: true
    check_puppeteer: true
    check_selenium: true
    check_playwright: true

# Risk Scoring
risk_scoring:
  weights:
    tls_mismatch: 0.9
    automation_detected: 1.0
    headless_detected: 0.95
    behavioral_anomaly: 0.7
    rate_limit_exceeded: 0.6
  
  thresholds:
    low: 0.3
    medium: 0.6
    high: 0.8
    critical: 0.95

# Simulation & Ambience
simulation:
  total_virtual_sites: 1000000
  default_tech_stacks: ["WordPress", "Shopify", "Drupal"]
  waf_enabled: true
  geo_rep_enabled: true
  site_profiles_seed: 12345

traffic:
  users_per_site_avg: 100
  device_diversity:
    mobile: 0.6
    desktop: 0.35
    tablet: 0.05

# Chaos Engineering
chaos:
  latency_spike_chance: 0.1
  error_chance: 0.05
  states:
    healthy: 0.7
    degraded: 0.15
    under_load: 0.1
    crashing: 0.05

# Infrastructure
infrastructure:
  storage: "redis"  # or "memory"
  redis:
    host: "localhost:6379"
    password: ""
    db: 0
  
  rate_limiting:
    requests_per_minute: 60
    burst: 10
```

## Dashboard Integration Architecture

### Adversarial Control Interface

```typescript
// Adversarial server control interface
export class AdversarialServerClient {
  private baseURL: string;
  private websocket: WebSocket | null = null;

  constructor(baseURL: string = 'http://localhost:8080') {
    this.baseURL = baseURL;
  }

  // Server control methods
  async getStatus(): Promise<ServerStatus> {
    const response = await fetch(`${this.baseURL}/status`);
    return response.json();
  }

  async updateConfig(config: Partial<AdversarialConfig>): Promise<void> {
    const response = await fetch(`${this.baseURL}/config`, {
      method: 'PUT',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(config),
    });
    
    if (!response.ok) {
      throw new Error('Failed to update configuration');
    }
  }

  async startSimulation(scenario: string, intensity: number): Promise<void> {
    const response = await fetch(`${this.baseURL}/simulation/start`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ scenario, intensity }),
    });
    
    if (!response.ok) {
      throw new Error('Failed to start simulation');
    }
  }

  async stopSimulation(): Promise<void> {
    const response = await fetch(`${this.baseURL}/simulation/stop`, {
      method: 'POST',
    });
    
    if (!response.ok) {
      throw new Error('Failed to stop simulation');
    }
  }

  // WebSocket for real-time updates
  connectWebSocket(): void {
    this.websocket = new WebSocket(`ws://localhost:8080/ws`);
    
    this.websocket.onmessage = (event) => {
      const data = JSON.parse(event.data);
      this.handleWebSocketMessage(data);
    };
  }

  private handleWebSocketMessage(data: any): void {
    switch (data.type) {
      case 'detection_event':
        this.handleDetectionEvent(data.event);
        break;
      case 'risk_score_update':
        this.handleRiskScoreUpdate(data.score);
        break;
      case 'simulation_status':
        this.handleSimulationStatus(data.status);
        break;
    }
  }

  private handleDetectionEvent(event: DetectionEvent): void {
    // Emit event for dashboard components
    window.dispatchEvent(new CustomEvent('detectionEvent', { detail: event }));
  }

  private handleRiskScoreUpdate(score: RiskScore): void {
    window.dispatchEvent(new CustomEvent('riskScoreUpdate', { detail: score }));
  }

  private handleSimulationStatus(status: SimulationStatus): void {
    window.dispatchEvent(new CustomEvent('simulationStatus', { detail: status }));
  }
}
```

## Detection Module Controls

### TLS Detection Configuration

```typescript
// TLS detection control panel
export const TLSConfiguration: React.FC = () => {
  const [config, setConfig] = useState<TLSConfig>({
    enabled: true,
    ja3Database: './fingerprints/ja3.json',
    customFingerprints: [],
  });

  const updateTLSConfig = async (updates: Partial<TLSConfig>) => {
    const newConfig = { ...config, ...updates };
    
    try {
      await adversarialClient.updateConfig({
        detection: {
          tls: newConfig,
        },
      });
      
      setConfig(newConfig);
      toast.success('TLS detection configuration updated');
    } catch (error) {
      toast.error('Failed to update TLS configuration');
    }
  };

  return (
    <div className="tls-configuration">
      <h3>TLS Detection</h3>
      
      <div className="config-section">
        <Toggle
          checked={config.enabled}
          onChange={(enabled) => updateTLSConfig({ enabled })}
          label="Enable TLS Detection"
        />
      </div>
      
      {config.enabled && (
        <div className="tls-settings">
          <div className="setting-row">
            <label htmlFor="ja3-database">JA3 Database Path:</label>
            <Input
              id="ja3-database"
              value={config.ja3Database}
              onChange={(e) => updateTLSConfig({ ja3Database: e.target.value })}
              placeholder="./fingerprints/ja3.json"
            />
          </div>
          
          <div className="setting-row">
            <label>Custom Fingerprints:</label>
            <FingerprintList
              fingerprints={config.customFingerprints}
              onAdd={(fingerprint) => 
                updateTLSConfig({
                  customFingerprints: [...config.customFingerprints, fingerprint],
                })
              }
              onRemove={(index) => {
                const updated = config.customFingerprints.filter((_, i) => i !== index);
                updateTLSConfig({ customFingerprints: updated });
              }}
            />
          </div>
          
          <Button
            onClick={() => testTLSDetection()}
            variant="secondary"
          >
            Test TLS Detection
          </Button>
        </div>
      )}
    </div>
  );
};

const testTLSDetection = async () => {
  try {
    const result = await adversarialClient.testTLSDetection();
    if (result.detected) {
      toast.success(`Detected ${result.fingerprints.length} unique TLS fingerprints`);
    } else {
      toast.warning('No TLS fingerprints detected');
    }
  } catch (error) {
    toast.error('Failed to test TLS detection');
  }
};
```

### Behavioral Analysis Configuration

```typescript
// Behavioral analysis controls
export const BehavioralAnalysisConfig: React.FC = () => {
  const [config, setConfig] = useState<BehavioralConfig>({
    mouseTracking: true,
    timingAnalysis: true,
    minMouseEvents: 3,
    maxPowDurationMs: 5000,
  });

  const updateBehavioralConfig = async (updates: Partial<BehavioralConfig>) => {
    const newConfig = { ...config, ...updates };
    
    try {
      await adversarialClient.updateConfig({
        detection: {
          behavioral: newConfig,
        },
      });
      
      setConfig(newConfig);
      toast.success('Behavioral analysis configuration updated');
    } catch (error) {
      toast.error('Failed to update behavioral analysis configuration');
    }
  };

  return (
    <div className="behavioral-analysis-config">
      <h3>Behavioral Analysis</h3>
      
      <div className="config-section">
        <Toggle
          checked={config.mouseTracking}
          onChange={(enabled) => updateBehavioralConfig({ mouseTracking: enabled })}
          label="Enable Mouse Tracking"
        />
        
        <Toggle
          checked={config.timingAnalysis}
          onChange={(enabled) => updateBehavioralConfig({ timingAnalysis: enabled })}
          label="Enable Timing Analysis"
        />
      </div>
      
      <div className="config-section">
        <label htmlFor="min-mouse-events">
          Minimum Mouse Events:
        </label>
        <Slider
          id="min-mouse-events"
          min={1}
          max={20}
          step={1}
          value={config.minMouseEvents}
          onChange={(value) => updateBehavioralConfig({ minMouseEvents: value })}
          marks={[
            { value: 1, label: '1' },
            { value: 5, label: '5' },
            { value: 10, label: '10' },
            { value: 15, label: '15' },
            { value: 20, label: '20' },
          ]}
        />
      </div>
      
      <div className="config-section">
        <label htmlFor="max-pow-duration">
          Max Proof-of-Work Duration:
        </label>
        <Slider
          id="max-pow-duration"
          min={1000}
          max={10000}
          step={500}
          value={config.maxPowDurationMs}
          onChange={(value) => updateBehavioralConfig({ maxPowDurationMs: value })}
          marks={[
            { value: 1000, label: '1s' },
            { value: 2500, label: '2.5s' },
            { value: 5000, label: '5s' },
            { value: 7500, label: '7.5s' },
            { value: 10000, label: '10s' },
          ]}
        />
      </div>
      
      <Button
        onClick={() => simulateBehavioralTest()}
        variant="secondary"
      >
        Simulate Behavioral Test
      </Button>
    </div>
  );
};

const simulateBehavioralTest = async () => {
  try {
    const result = await adversarialClient.simulateBehavioralTest();
    toast.success(`Behavioral analysis detected ${result.anomalies.length} anomalies`);
  } catch (error) {
    toast.error('Failed to run behavioral test');
  }
};
```

### Risk Scoring Configuration

```typescript
// Risk scoring control panel
export const RiskScoringConfig: React.FC = () => {
  const [weights, setWeights] = useState<RiskWeights>({
    tlsMismatch: 0.9,
    automationDetected: 1.0,
    headlessDetected: 0.95,
    behavioralAnomaly: 0.7,
    rateLimitExceeded: 0.6,
  });

  const [thresholds, setThresholds] = useState<RiskThresholds>({
    low: 0.3,
    medium: 0.6,
    high: 0.8,
    critical: 0.95,
  });

  const updateWeights = async (newWeights: Partial<RiskWeights>) => {
    const updatedWeights = { ...weights, ...newWeights };
    
    try {
      await adversarialClient.updateConfig({
        riskScoring: {
          weights: updatedWeights,
        },
      });
      
      setWeights(updatedWeights);
      toast.success('Risk scoring weights updated');
    } catch (error) {
      toast.error('Failed to update risk scoring weights');
    }
  };

  const updateThresholds = async (newThresholds: Partial<RiskThresholds>) => {
    const updatedThresholds = { ...thresholds, ...newThresholds };
    
    try {
      await adversarialClient.updateConfig({
        riskScoring: {
          thresholds: updatedThresholds,
        },
      });
      
      setThresholds(updatedThresholds);
      toast.success('Risk scoring thresholds updated');
    } catch (error) {
      toast.error('Failed to update risk scoring thresholds');
    }
  };

  return (
    <div className="risk-scoring-config">
      <h3>Risk Scoring Configuration</h3>
      
      <div className="config-section">
        <h4>Detection Weights</h4>
        
        <div className="weight-controls">
          <WeightSlider
            label="TLS Mismatch"
            value={weights.tlsMismatch}
            onChange={(value) => updateWeights({ tlsMismatch: value })}
          />
          
          <WeightSlider
            label="Automation Detected"
            value={weights.automationDetected}
            onChange={(value) => updateWeights({ automationDetected: value })}
          />
          
          <WeightSlider
            label="Headless Detected"
            value={weights.headlessDetected}
            onChange={(value) => updateWeights({ headlessDetected: value })}
          />
          
          <WeightSlider
            label="Behavioral Anomaly"
            value={weights.behavioralAnomaly}
            onChange={(value) => updateWeights({ behavioralAnomaly: value })}
          />
          
          <WeightSlider
            label="Rate Limit Exceeded"
            value={weights.rateLimitExceeded}
            onChange={(value) => updateWeights({ rateLimitExceeded: value })}
          />
        </div>
      </div>
      
      <div className="config-section">
        <h4>Risk Thresholds</h4>
        
        <div className="threshold-controls">
          <ThresholdSlider
            label="Low Risk"
            value={thresholds.low}
            onChange={(value) => updateThresholds({ low: value })}
            color="green"
          />
          
          <ThresholdSlider
            label="Medium Risk"
            value={thresholds.medium}
            onChange={(value) => updateThresholds({ medium: value })}
            color="yellow"
          />
          
          <ThresholdSlider
            label="High Risk"
            value={thresholds.high}
            onChange={(value) => updateThresholds({ high: value })}
            color="orange"
          />
          
          <ThresholdSlider
            label="Critical Risk"
            value={thresholds.critical}
            onChange={(value) => updateThresholds({ critical: value })}
            color="red"
          />
        </div>
      </div>
      
      <div className="risk-visualization">
        <RiskScoreGauge
          weights={weights}
          thresholds={thresholds}
          currentScore={0.75} // Example current score
        />
      </div>
    </div>
  );
};
```

## Simulation Controls

### Scenario Management

```typescript
// Adversarial simulation scenarios
export const SimulationScenarios: React.FC = () => {
  const [scenarios] = useState<SimulationScenario[]>([
    {
      id: 'basic_bot_detection',
      name: 'Basic Bot Detection',
      description: 'Standard bot detection techniques',
      defaultIntensity: 50,
      modules: ['tls', 'behavioral', 'automation'],
    },
    {
      id: 'advanced_fingerprinting',
      name: 'Advanced Fingerprinting',
      description: 'Comprehensive browser fingerprinting',
      defaultIntensity: 75,
      modules: ['tls', 'http2', 'fingerprinting', 'behavioral'],
    },
    {
      id: 'rate_limiting_aggressive',
      name: 'Aggressive Rate Limiting',
      description: 'Strict rate limiting with CAPTCHAs',
      defaultIntensity: 90,
      modules: ['rate_limiting', 'behavioral'],
    },
    {
      id: 'waf_protection',
      name: 'WAF Protection',
      description: 'Web Application Firewall simulation',
      defaultIntensity: 80,
      modules: ['waf', 'rate_limiting', 'tls'],
    },
    {
      id: 'honeypot_traps',
      name: 'Honeypot Traps',
      description: 'Hidden traps and deception techniques',
      defaultIntensity: 60,
      modules: ['honeypot', 'behavioral', 'automation'],
    },
  ]);

  const [selectedScenario, setSelectedScenario] = useState<string | null>(null);
  const [intensity, setIntensity] = useState(50);
  const [isRunning, setIsRunning] = useState(false);

  const startSimulation = async () => {
    if (!selectedScenario) {
      toast.error('Please select a simulation scenario');
      return;
    }

    setIsRunning(true);
    try {
      await adversarialClient.startSimulation(selectedScenario, intensity);
      toast.success(`Started ${selectedScenario} simulation`);
    } catch (error) {
      toast.error('Failed to start simulation');
      setIsRunning(false);
    }
  };

  const stopSimulation = async () => {
    try {
      await adversarialClient.stopSimulation();
      toast.success('Simulation stopped');
      setIsRunning(false);
    } catch (error) {
      toast.error('Failed to stop simulation');
    }
  };

  return (
    <div className="simulation-scenarios">
      <h3>Adversarial Simulation Scenarios</h3>
      
      <div className="scenario-grid">
        {scenarios.map((scenario) => (
          <Card
            key={scenario.id}
            className={`scenario-card ${selectedScenario === scenario.id ? 'selected' : ''}`}
            onClick={() => setSelectedScenario(scenario.id)}
          >
            <h4>{scenario.name}</h4>
            <p>{scenario.description}</p>
            
            <div className="scenario-modules">
              <strong>Modules:</strong>
              <ul>
                {scenario.modules.map((module) => (
                  <li key={module}>{module}</li>
                ))}
              </ul>
            </div>
            
            <div className="scenario-intensity">
              <strong>Default Intensity:</strong> {scenario.defaultIntensity}%
            </div>
          </Card>
        ))}
      </div>
      
      {selectedScenario && (
        <div className="simulation-controls">
          <h4>Simulation Controls</h4>
          
          <div className="intensity-control">
            <label htmlFor="intensity-slider">
              Intensity: {intensity}%
            </label>
            <Slider
              id="intensity-slider"
              min={0}
              max={100}
              step={5}
              value={intensity}
              onChange={setIntensity}
              marks={[
                { value: 0, label: 'Off' },
                { value: 25, label: 'Low' },
                { value: 50, label: 'Medium' },
                { value: 75, label: 'High' },
                { value: 100, label: 'Extreme' },
              ]}
            />
          </div>
          
          <div className="control-buttons">
            {!isRunning ? (
              <Button
                onClick={startSimulation}
                variant="primary"
                size="large"
              >
                Start Simulation
              </Button>
            ) : (
              <Button
                onClick={stopSimulation}
                variant="danger"
                size="large"
              >
                Stop Simulation
              </Button>
            )}
          </div>
        </div>
      )}
    </div>
  );
};
```

### Chaos Engineering Controls

```typescript
// Chaos engineering configuration
export const ChaosEngineeringConfig: React.FC = () => {
  const [chaosConfig, setChaosConfig] = useState<ChaosConfig>({
    latencySpikeChance: 0.1,
    errorChance: 0.05,
    states: {
      healthy: 0.7,
      degraded: 0.15,
      underLoad: 0.1,
      crashing: 0.05,
    },
  });

  const updateChaosConfig = async (updates: Partial<ChaosConfig>) => {
    const newConfig = { ...chaosConfig, ...updates };
    
    try {
      await adversarialClient.updateConfig({
        chaos: newConfig,
      });
      
      setChaosConfig(newConfig);
      toast.success('Chaos engineering configuration updated');
    } catch (error) {
      toast.error('Failed to update chaos configuration');
    }
  };

  return (
    <div className="chaos-engineering-config">
      <h3>Chaos Engineering</h3>
      
      <div className="config-section">
        <label htmlFor="latency-spike-chance">
          Latency Spike Probability: {(chaosConfig.latencySpikeChance * 100).toFixed(0)}%
        </label>
        <Slider
          id="latency-spike-chance"
          min={0}
          max={1}
          step={0.05}
          value={chaosConfig.latencySpikeChance}
          onChange={(value) => updateChaosConfig({ latencySpikeChance: value })}
          marks={[
            { value: 0, label: '0%' },
            { value: 0.25, label: '25%' },
            { value: 0.5, label: '50%' },
            { value: 0.75, label: '75%' },
            { value: 1, label: '100%' },
          ]}
        />
      </div>
      
      <div className="config-section">
        <label htmlFor="error-chance">
          Error Probability: {(chaosConfig.errorChance * 100).toFixed(0)}%
        </label>
        <Slider
          id="error-chance"
          min={0}
          max={1}
          step={0.05}
          value={chaosConfig.errorChance}
          onChange={(value) => updateChaosConfig({ errorChance: value })}
          marks={[
            { value: 0, label: '0%' },
            { value: 0.25, label: '25%' },
            { value: 0.5, label: '50%' },
            { value: 0.75, label: '75%' },
            { value: 1, label: '100%' },
          ]}
        />
      </div>
      
      <div className="config-section">
        <h4>Server State Distribution</h4>
        
        <StateDistributionSlider
          label="Healthy"
          value={chaosConfig.states.healthy}
          color="green"
          onChange={(value) => updateChaosConfig({
            states: { ...chaosConfig.states, healthy: value }
          })}
        />
        
        <StateDistributionSlider
          label="Degraded"
          value={chaosConfig.states.degraded}
          color="yellow"
          onChange={(value) => updateChaosConfig({
            states: { ...chaosConfig.states, degraded: value }
          })}
        />
        
        <StateDistributionSlider
          label="Under Load"
          value={chaosConfig.states.underLoad}
          color="orange"
          onChange={(value) => updateChaosConfig({
            states: { ...chaosConfig.states, underLoad: value }
          })}
        />
        
        <StateDistributionSlider
          label="Crashing"
          value={chaosConfig.states.crashing}
          color="red"
          onChange={(value) => updateChaosConfig({
            states: { ...chaosConfig.states, crashing: value }
          })}
        />
      </div>
    </div>
  );
};
```

## Real-Time Monitoring

### Detection Event Stream

```typescript
// Real-time detection events
export const DetectionEventStream: React.FC = () => {
  const [events, setEvents] = useState<DetectionEvent[]>([]);
  const [filter, setFilter] = useState<string>('all');

  useEffect(() => {
    const handleDetectionEvent = (event: CustomEvent) => {
      setEvents((prevEvents) => [event.detail, ...prevEvents.slice(0, 99)]);
    };

    window.addEventListener('detectionEvent', handleDetectionEvent as EventListener);
    
    return () => {
      window.removeEventListener('detectionEvent', handleDetectionEvent as EventListener);
    };
  }, []);

  const filteredEvents = events.filter((event) => {
    if (filter === 'all') return true;
    return event.type === filter;
  });

  return (
    <div className="detection-event-stream">
      <h3>Detection Events</h3>
      
      <div className="filter-controls">
        <Select
          value={filter}
          onChange={(value) => setFilter(value)}
          options={[
            { value: 'all', label: 'All Events' },
            { value: 'tls', label: 'TLS' },
            { value: 'behavioral', label: 'Behavioral' },
            { value: 'automation', label: 'Automation' },
            { value: 'fingerprinting', label: 'Fingerprinting' },
          ]}
        />
      </div>
      
      <div className="event-list">
        {filteredEvents.map((event, index) => (
          <Alert
            key={index}
            type={event.severity === 'high' ? 'error' : 'warning'}
            title={`${event.type.toUpperCase()} Detection`}
            description={event.message}
            timestamp={event.timestamp}
            actions={[
              {
                label: 'Details',
                onClick: () => showEventDetails(event),
              },
            ]}
          />
        ))}
      </div>
    </div>
  );
};
```

### Risk Score Visualization

```typescript
// Risk score gauge and history
export const RiskScoreMonitor: React.FC = () => {
  const [currentScore, setCurrentScore] = useState(0.0);
  const [scoreHistory, setScoreHistory] = useState<number[]>([]);

  useEffect(() => {
    const handleRiskScoreUpdate = (event: CustomEvent) => {
      const score = event.detail.score;
      setCurrentScore(score);
      setScoreHistory((prev) => [...prev.slice(-99), score]);
    };

    window.addEventListener('riskScoreUpdate', handleRiskScoreUpdate as EventListener);
    
    return () => {
      window.removeEventListener('riskScoreUpdate', handleRiskScoreUpdate as EventListener);
    };
  }, []);

  const getRiskLevel = (score: number): RiskLevel => {
    if (score >= 0.95) return 'critical';
    if (score >= 0.8) return 'high';
    if (score >= 0.6) return 'medium';
    if (score >= 0.3) return 'low';
    return 'minimal';
  };

  const riskLevel = getRiskLevel(currentScore);

  return (
    <div className="risk-score-monitor">
      <h3>Risk Score Monitor</h3>
      
      <div className="current-score">
        <Gauge
          value={currentScore}
          min={0}
          max={1}
          color={getRiskColor(riskLevel)}
          label={`Current Risk: ${(currentScore * 100).toFixed(1)}%`}
        />
        
        <div className={`risk-level ${riskLevel}`}>
          {riskLevel.toUpperCase()} RISK
        </div>
      </div>
      
      <div className="score-history">
        <h4>Risk Score History</h4>
        <SparklineChart
          data={scoreHistory}
          color={getRiskColor(riskLevel)}
          height={100}
        />
      </div>
      
      <div className="risk-factors">
        <h4>Contributing Factors</h4>
        <RiskFactorList score={currentScore} />
      </div>
    </div>
  );
};
```

This integration provides comprehensive control and monitoring capabilities for the Go adversarial server through the AEGIS Dashboard, enabling users to simulate various bot detection scenarios and test scraper resilience in real-time.
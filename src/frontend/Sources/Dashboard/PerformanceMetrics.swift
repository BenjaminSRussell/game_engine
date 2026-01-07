import SwiftUI
import Charts

struct PerformanceMetricsView: View {
    
    struct MetricPoint: Identifiable {
        let id = UUID()
        let time: TimeInterval
        let value: Double
    }
    
    @State private var fpsData: [MetricPoint] = []
    @State private var memoryData: [MetricPoint] = []
    @State private var drawCalls: Int = 0
    @State private var cpuTime: Double = 0.0
    
    let timer = Timer.publish(every: 1.0, on: .main, in: .common).autoconnect()
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Engine Performance")
                .font(.title3)
                .padding(.bottom, 8)
            
            HStack(spacing: 20) {
                MetricCard(title: "FPS", value: String(format: "%.0f", fpsData.last?.value ?? 60), unit: "", color: .green)
                MetricCard(title: "Memory", value: String(format: "%.1f", (memoryData.last?.value ?? 0)), unit: "MB", color: .blue)
                MetricCard(title: "Draw Calls", value: "\(drawCalls)", unit: "", color: .purple)
                MetricCard(title: "CPU Time", value: String(format: "%.2f", cpuTime), unit: "ms", color: .orange)
            }
            
            Text("Frame History")
                .font(.headline)
            
            Chart(fpsData) {
                LineMark(
                    x: .value("Time", $0.time),
                    y: .value("FPS", $0.value)
                )
                .foregroundStyle(.green)
            }
            .frame(height: 200)
            .chartYScale(domain: 0...144)
            
            Text("Memory Usage")
                .font(.headline)
                
            Chart(memoryData) {
                AreaMark(
                    x: .value("Time", $0.time),
                    y: .value("MB", $0.value)
                )
                .foregroundStyle(.blue.opacity(0.2))
                .interpolationMethod(.catmullRom)
                
                LineMark(
                    x: .value("Time", $0.time),
                    y: .value("MB", $0.value)
                )
                .foregroundStyle(.blue)
            }
            .frame(height: 150)
        }
        .padding()
        .onReceive(timer) { _ in
            updateMockData()
        }
        .onAppear {
            initializeMockData()
        }
    }
    
    private func initializeMockData() {
        let now = Date().timeIntervalSinceReferenceDate
        for i in 0..<60 {
            fpsData.append(MetricPoint(time: now - Double(60 - i), value: 60 + Double.random(in: -5...5)))
            memoryData.append(MetricPoint(time: now - Double(60 - i), value: 256 + Double.random(in: -10...20)))
        }
    }
    
    private func updateMockData() {
        let now = Date().timeIntervalSinceReferenceDate
        
        // Simulate engine data
        let newFPS = 60 + Double.random(in: -10...5)
        let newMem = (memoryData.last?.value ?? 256) + Double.random(in: -5...5)
        
        fpsData.append(MetricPoint(time: now, value: newFPS))
        memoryData.append(MetricPoint(time: now, value: newMem))
        
        if fpsData.count > 60 { fpsData.removeFirst() }
        if memoryData.count > 60 { memoryData.removeFirst() }
        
        drawCalls = Int.random(in: 150...400)
        cpuTime = Double.random(in: 8...16)
    }
}

struct MetricCard: View {
    let title: String
    let value: String
    let unit: String
    let color: Color
    
    var body: some View {
        VStack(alignment: .leading) {
            Text(title)
                .font(.caption)
                .foregroundColor(.secondary)
            HStack(alignment: .lastTextBaseline) {
                Text(value)
                    .font(.system(size: 24, weight: .bold))
                    .foregroundColor(color)
                Text(unit)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(8)
    }
}

struct PerformanceMetricsView_Previews: PreviewProvider {
    static var previews: some View {
        PerformanceMetricsView()
    }
}

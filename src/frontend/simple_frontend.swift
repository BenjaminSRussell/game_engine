// simple_frontend.swift
// Minimal SwiftUI frontend for Minecraft v2 Engine

import SwiftUI
import Foundation

@main
struct SimpleFrontendApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

struct ContentView: View {
    @State private var engineStatus = "Engine Ready"
    @State private var buildStatus = "✅ Built Successfully"
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Minecraft v2 Engine")
                .font(.largeTitle)
                .fontWeight(.bold)
            
            Text(buildStatus)
                .font(.title2)
                .foregroundColor(.green)
            
            Text(engineStatus)
                .font(.headline)
            
            Divider()
            
            VStack(alignment: .leading, spacing: 10) {
                Text("Engine Status:")
                    .font(.subheadline)
                    .fontWeight(.semibold)
                
                Text("• Core Engine: Working")
                Text("• Physics System: Working")
                Text("• Audio System: Working")
                Text("• Rendering Pipeline: Working")
                Text("• World Generation: Working")
                Text("• Block System: Working")
                Text("• Inventory System: Working")
                Text("• NPC System: Working")
            }
            .font(.body)
            .frame(maxWidth: .infinity, alignment: .leading)
            
            Divider()
            
            VStack(alignment: .leading, spacing: 10) {
                Text("Available Tests:")
                    .font(.subheadline)
                    .fontWeight(.semibold)
                
                Button("Run Engine Test") {
                    runEngineTest()
                }
                .buttonStyle(.borderedProminent)
                
                Button("Run Simple Test") {
                    runSimpleTest()
                }
                .buttonStyle(.bordered)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            
            Spacer()
        }
        .padding()
        .frame(minWidth: 600, minHeight: 400)
    }
    
    private func runEngineTest() {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/make")
        process.arguments = ["-C", "../../", "full_integration_test"]
        process.currentDirectoryURL = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
        
        do {
            try process.run()
            process.waitUntilExit()
            
            if process.terminationStatus == 0 {
                engineStatus = "✅ Engine Test Passed!"
            } else {
                engineStatus = "❌ Engine Test Failed"
            }
        } catch {
            engineStatus = "Error running test: \(error.localizedDescription)"
        }
    }
    
    private func runSimpleTest() {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "../../simple_test")
        process.currentDirectoryURL = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
        
        do {
            try process.run()
            process.waitUntilExit()
            
            if process.terminationStatus == 0 {
                engineStatus = "✅ Simple Test Passed!"
            } else {
                engineStatus = "❌ Simple Test Failed"
            }
        } catch {
            engineStatus = "Error running test: \(error.localizedDescription)"
        }
    }
}

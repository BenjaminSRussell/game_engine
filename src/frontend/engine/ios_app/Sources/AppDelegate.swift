// AppDelegate.swift
//
// Purpose: This file serves as the main entry point for the UIKit application.
// It handles application-level lifecycle events, such as the app launch and
// the configuration of new scenes.
//
// Role: Manages the initial setup of the application and provides the
// configuration for `UISceneSession`s, directing to `SceneDelegate` for
// scene-specific lifecycle management.
//
// Lifecycle:
// - `application(_:didFinishLaunchingWithOptions:)`: Called when the app has launched.
// - `application(_:configurationForConnecting:options:)`: Called when a new scene session is being created.
//
// Inputs: Application launch options, connection options for new scenes.
// Outputs: Returns `true` for successful launch, returns a `UISceneConfiguration` for new scenes.
//
import UIKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
    
    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        return true
    }
    
    func application(_ application: UIApplication, configurationForConnecting connectingSceneSession: UISceneSession, options: UIScene.ConnectionOptions) -> UISceneConfiguration {
        let configuration = UISceneConfiguration(name: "Default Configuration", sessionRole: connectingSceneSession.role)
        configuration.delegateClass = SceneDelegate.self
        return configuration
    }
}

// SceneDelegate.swift
//
// Purpose: This file manages the lifecycle of a specific UI scene in the application.
// It is responsible for setting up the initial user interface, including creating
// the main window and configuring its root view controller.
//
// Role: Coordinates the presentation of the app's content within a given scene.
// It instantiates the main `UIWindow` and sets the `MainMenuViewController` as
// the root of a `UINavigationController` to establish the primary navigation flow.
//
// Lifecycle:
// - `scene(_:willConnectTo:options:)`: Called when a scene is being connected
//   to the application, where the main window and view hierarchy are established.
//
// Inputs: A `UIScene` object, a `UISceneSession` object, and `UIScene.ConnectionOptions`.
// Outputs: Configures and displays the application's UI within the connected scene.
//
import UIKit

class SceneDelegate: UIResponder, UIWindowSceneDelegate {
    
    var window: UIWindow?
    
    func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options connectionOptions: UIScene.ConnectionOptions) {
        guard let windowScene = (scene as? UIWindowScene) else { return }
        
        let window = UIWindow(windowScene: windowScene)
        let navigationController = UINavigationController(rootViewController: MainMenuViewController())
        window.rootViewController = navigationController
        self.window = window
        window.makeKeyAndVisible()
    }
}

# RoadNetworkTool
The Road Network Tool is an Unreal Engine plugin for creating complex road layouts using splines. Easily add, modify, and shape roads by adjusting spline points in the editor, while the tool generates road meshes and includes built-in AI pathfinding for vehicles. Ideal for smooth, curved roads and interconnected networks.

# Key Features
- **Spline-based Road Creation**: Create and shape road layouts by adding and adjusting spline points in the editor.
- **Procedural Mesh Generation**: Automatically generate road meshes according to the configured spline shapes and road width.
- **AI Pathfinding Support**: Built-in pathfinding for vehicle AI to navigate through the generated road networks.
- **Road Segment Connection**: Seamlessly connect and manage road segments for building complex networks.
- **Efficient Spatial Queries**: Use Quadtree-based spatial queries for improved road network performance.
- **User-friendly Editor Plugin**: Intuitive interface and controls for creating, managing, and visualizing road networks.

# Installation
1. Download and unzip the plugin folder.
2. Move the folder into your Unreal Engine project's Plugins directory.
3. Launch Unreal Engine and enable the Road Network Tool in the Plugins menu.
4. Restart Unreal Engine if prompted.

# Getting Started
1. **Switch to the Road Network Tool Mode**: Click on **Selection Mode** and switch to **RoadNetworkTool Mode**.
2. **Create Splines**: While in **Line Tool** mode, click on the floor or landscape to add new spline points.
3. **Connect and Shape Roads**: Connect road segments to shape the network as needed. Switch to the **Curve Tool**, click on the spline, and drag to create curved roads.
4. **Generate Road Meshes**: Adjust the road width as needed and click **Create** button to create procedural road surfaces based on the spline shapes.
5. **Enable AI Pathfinding**: In your vehicle blueprint, use the `Get Actor of Class` node, with the actor class set to `Road Actor`. Call the `Find Path Road Network` node. This will return an array of location paths from the `startLocation` to the `targetLocation`.
# Requirements
Unreal Engine Version: 5.4 later.
Development Environment: Unreal Engine Editor with C++ support (optional for source code modifications).

# Support
For bug reports, feature requests, or general questions, please visit our [Patreon Page](https://www.patreon.com/jourverse).

# License
This project is licensed under the MIT License - see the [LICENSE.md](https://github.com/sengchor/RoadNetworkTool/blob/main/LICENSE) file for details.

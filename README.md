# Dungeon Vibe

A retro-styled 3D dungeon exploration game built with modern OpenGL graphics and atmospheric rendering.

## Overview

**Dungeon Vibe** is an immersive first-person dungeon crawler featuring a nostalgic retro aesthetic combined with contemporary rendering techniques. Navigate through procedurally generated dungeons with a focus on atmospheric presentation, dynamic lighting, and a tactical fog-of-war minimap.

## Features

### Gameplay
- **First-Person Exploration**: Navigate dungeons with smooth WASD movement and mouse-look controls
- **Procedural Map Generation**: Dynamically generated dungeon layouts using room-based generation
- **Fog of War System**: Strategic visibility system that reveals explored areas on the minimap
- **Collision Detection**: Realistic player physics with wall and obstacle collision
- **Dynamic Lighting**: Torch-based ambient lighting with flickering effects

### Visual Systems
- **Low-Resolution Rendering with Upscaling**: Renders at 320x180 and upscales for retro aesthetic
- **Screen-Space Ambient Occlusion (SSAO)**: Realistic shadowing for depth and atmosphere
- **Dithering Post-Processing**: Customizable dither patterns for authentic retro feel
- **Anisotropic Texture Filtering**: High-quality texture sampling
- **Minimap Overlay**: Real-time tactical map with transparency control

### Technical
- **OpenGL 3.3 Core**: Modern graphics pipeline
- **Shader-Based Rendering**: Custom vertex and fragment shaders for visual effects
- **Framebuffer Objects**: Multi-pass rendering for post-processing effects
- **Configuration System**: Runtime settings for tweaking visual parameters

## Building

### Requirements
- C++ compiler (G++)
- OpenGL 3.3+ support
- Dependencies:
  - GLEW (OpenGL Extension Wrangler)
  - GLFW3 (Window and input management)
  - GLM (OpenGL Mathematics)

### Linux / MinGW
```bash
make build
```

### Running
```bash
make run
```

## Controls

| Key | Action |
|-----|--------|
| **W/A/S/D** | Move forward/left/backward/right |
| **Mouse** | Look around |
| **TAB** | Toggle minimap transparency (full-screen mode) |
| **ESC** | Exit game |

## Project Structure

```
├── main.cpp              # Main game loop and renderer initialization
├── Camera.h              # First-person camera system with input handling
├── Map.h                 # Dungeon generation and geometry
├── Minimap.h             # Real-time minimap rendering
├── Display.h             # Display and window management
├── Shaders.h             # Shader source code definitions
├── ShaderUtils.h         # Shader compilation and linking utilities
├── Textures.h            # Texture loading and management
├── Settings.h            # Configuration and visual parameters
├── Input.h               # Input callbacks (keyboard, mouse)
├── floor.png             # Floor/ceiling texture
├── wall.png              # Wall texture
└── settings.cfg          # Configuration file
```

## Documentation

For detailed information about game mechanics and visual systems, see the [docs](docs/) directory:

- **[Game Mechanics](docs/game_mechanics.md)** - Camera system, collision detection, fog of war, and lighting
- **[Visual Systems](docs/visual_systems.md)** - Rendering pipeline, shaders, post-processing, and minimap

## Graphics Features

### Retro Aesthetic
The game achieves its retro look through several techniques:
- Low-resolution rendering (320x180) upscaled to match native resolution
- Ordered dithering patterns for color quantization
- Pixelated textures and intentional aliasing

### Modern Rendering
Despite the retro style, the rendering leverages modern OpenGL techniques:
- Multiple render passes with framebuffer objects
- Screen-space techniques for ambient occlusion
- Dynamic lighting with smooth falloff
- Efficient geometry management

## Configuration

Edit `settings.cfg` to customize:
- **FOV_DEGREES**: Field of view angle
- **AMBIENT_LIGHT**: Base ambient light level
- **LIGHT_RADIUS_NEAR/FAR**: Torch lighting distance and falloff
- **USE_DITHER**: Enable/disable dithering effects
- **DITHER_PALETTE**: Choose dither pattern
- **SCREEN_DISTORTION**: CRT-like distortion effect
- **ANISOTROPY_LEVEL**: Texture filtering quality

## License

This project is part of the Dungeon Vibe series.

## Contributing

Contributions are welcome! Please ensure code follows the existing style and include documentation for new features.

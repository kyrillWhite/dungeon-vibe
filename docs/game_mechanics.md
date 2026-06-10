# Game Mechanics

## Overview

Dungeon Vibe is a first-person exploration game with procedurally generated dungeons, tactical fog-of-war mechanics, and dynamic environmental interactions.

## Camera System

### First-Person Perspective
- **Position**: Tracks player location in 3D space
- **Height**: Fixed at 0.5 units above floor level
- **Look Direction**: Controlled by mouse movement with free rotation
- **Field of View**: Configurable via `FOV_DEGREES` (default: 60°)

### Mouse Controls
- **Yaw Rotation**: Horizontal rotation controlled by mouse X-axis movement
- **View Locking**: Cursor automatically locked to viewport during gameplay
- **Sensitivity**: Smooth framerate-independent input handling
- **ESC Key**: Toggle cursor lock state

### Movement Controls

| Control | Action |
|---------|--------|
| **W** | Move forward relative to view direction |
| **S** | Move backward |
| **A** | Strafe left |
| **D** | Strafe right |

Movement speed scales with frame delta time for framerate-independent gameplay.

## Collision System

### Player Collision
- **Collision Radius**: 0.2 units (sphere-based collision)
- **Detection**: Tests 4 corner points around player position against wall grid
- **Resolution**: Stops player movement if collision detected
- **Response**: Prevents movement into obstacles while allowing sliding along walls

### Wall Detection
- **Grid-Based**: Uses MAP_SIZE × MAP_SIZE grid (default: 128×128)
- **Cell Evaluation**: Checks if target cells contain walls (grid value = 1)
- **Boundary Safety**: Returns wall collision if movement goes outside map bounds
- **Real-Time**: Collision calculated every frame with delta time scaling

## Fog of War System

### Visibility Tracking
The minimap uses a fog-of-war system to track explored areas:

- **Visible Grid**: 128×128 visibility array tracking player-explored cells
- **Initial State**: All cells start as unexplored (0)
- **Exploration**: Cells become visible (1) when player explores them

### Visibility Calculation

**Ray-Based Raycast System**:
- Launches 120 rays from player position in all directions
- Each ray extends up to 6 units of distance
- Ray hits are checked at 0.3-unit intervals
- Walls stop ray propagation (grid value = 1)

**Visibility Update**:
1. Mark player's current cell as visible
2. Fire 120 raycasts in 3° increments around player
3. Mark all cells along visible rays as explored
4. Rays stop at wall boundaries
5. Update minimap display based on visibility state

### Strategic Implications
- Player can see tactical map of explored areas
- Walls block vision, creating tactical corridors
- Exploration rewards mapping the dungeon
- View distance of 6 units creates local awareness

## Dungeon Generation

### Room-Based Layout
Dungeons are generated using room placement algorithms:
- **Map Grid**: 128×128 cells
- **Room Distribution**: Multiple rooms placed with random dimensions
- **Corridor Generation**: Connections between rooms
- **Spawn Point**: Random location within first generated room

### Map Data Structure

```cpp
int grid[MAP_SIZE][MAP_SIZE];     // 0 = floor, 1 = wall
int visible[MAP_SIZE][MAP_SIZE];  // 0 = unexplored, 1 = visible
```

### Procedural Variation
- Seed-based generation ensures consistent dungeons per session
- Room sizes vary to create interesting layouts
- Random wall placement within procedural bounds

## Dynamic Lighting System

### Torch Light Source
**Type**: Player-carried dynamic light source
- **Color**: Warm yellow-orange (1.0, 0.55, 0.18)
- **Falloff**: Smooth distance-based attenuation

### Lighting Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `LIGHT_RADIUS_NEAR` | Minimum effective light distance | 0.5 |
| `LIGHT_RADIUS_FAR` | Maximum light reach distance | 20.0 |
| `LIGHT_SHARPNESS` | Falloff curve steepness | 2.0 |
| `LIGHT_TRANSITION_SOFTNESS` | Smooth falloff gradient | 1.0 |
| `AMBIENT_LIGHT` | Base ambient brightness | 0.2 |

### Flickering Torch Effect

The torch produces organic, believable flickering:

**Flicker System**:
- **Flicker Range**: 0.78 - 1.18x base intensity
- **Change Interval**: 0.12 - 0.9 seconds between intensity shifts
- **Smoothing**: Exponential smoothing toward target intensity
- **Smoothing Speed**: 3.5x multiplier for responsive flickering
- **Bounds**: Clamped between 0.6x and 1.25x to maintain visibility

**Implementation**:
- Time-based update using frame delta time
- Independent of frame rate (60 FPS vs 144 FPS renders identically)
- Random RNG seeding for unique flicker patterns per session
- Exponential interpolation for smooth visual transitions

## Environmental Interaction

### Interactive Elements
- **Walls**: Solid collision obstacles
- **Floor/Ceiling**: Walkable surface with texture detail
- **Map Grid**: Determines all collision and visibility

### Audio-Visual Polish (Planned)
- Footstep sounds when walking on different surfaces
- Torch crackling ambient audio
- Wall impact/collision audio feedback

## Game State Management

### Runtime Configuration
Settings are loaded from `settings.cfg` on startup and can be modified in real-time via the Settings system. Changes persist on exit.

### Performance Considerations
- **Delta Time Scaling**: All movement and animation uses frame-time for consistent gameplay speed
- **Culling**: Off-screen geometry automatically culled via OpenGL
- **LOD Consideration**: Room-based generation allows future optimization

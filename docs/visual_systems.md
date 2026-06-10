# Visual Systems

## Rendering Pipeline Overview

Dungeon Vibe uses a multi-pass rendering pipeline to achieve its distinctive retro-aesthetic with modern visual quality:

```
┌─────────────────────────────────────────────────────┐
│ Frame Render Pass                                   │
├─────────────────────────────────────────────────────┤
│ 1. Low-Res 3D Rendering (320×180 framebuffer)      │
│    ├─ Wall geometry with textures                  │
│    ├─ Floor/ceiling geometry with textures         │
│    └─ Minimap overlay (transparent layer)          │
├─────────────────────────────────────────────────────┤
│ 2. SSAO + Dithering Post-Processing                │
│    ├─ Screen-Space Ambient Occlusion               │
│    ├─ Optional dithering for color reduction       │
│    └─ Output to intermediate framebuffer           │
├─────────────────────────────────────────────────────┤
│ 3. Upscaling to Screen Resolution                  │
│    ├─ Nearest-neighbor or linear filtering         │
│    ├─ CRT distortion effects (optional)            │
│    └─ Final presentation to framebuffer            │
└─────────────────────────────────────────────────────┘
```

## Framebuffer Architecture

### Primary Low-Resolution Framebuffer
- **Resolution**: 320×180 pixels
- **Color Attachment**: RGB8 texture
- **Depth Attachment**: Depth24 texture
- **Purpose**: Renders 3D geometry and minimap at low resolution

### SSAO Output Framebuffer
- **Resolution**: 320×180 pixels (matching low-res)
- **Color Attachment**: RGB8 texture (SSAO-processed result)
- **Purpose**: Stores ambient-occlusion-modified colors before upscaling

### Screen Framebuffer
- **Resolution**: Native window resolution (configurable)
- **Source**: SSAO framebuffer upscaled with optional distortion
- **Purpose**: Final output to display

## Shader System

### 3D Rendering Shader (Vertex + Fragment)

**Purpose**: Renders all 3D geometry with lighting and texturing

**Vertex Processing**:
- Transform vertex positions by model-view-projection matrices
- Pass texture coordinates and AO values to fragment shader
- Preserve world-space position for lighting calculations

**Fragment Processing**:
- Sample texture at interpolated UV coordinates
- Calculate distance-based torch lighting
- Apply ambient light base
- Modulate with pre-baked ambient occlusion (AO)
- Final output: Base color × (ambient + torch light) × AO

**Key Uniforms**:
- `model`, `view`, `projection`: Transformation matrices
- `cameraPos`: Player eye position for lighting calculations
- `lightNear`, `lightFar`: Torch lighting distance range
- `lightSharpness`: Falloff curve steepness
- `lightTransitionSoftness`: Smooth transition gradient
- `torchColor`: Torch RGB color (warm orange-yellow)
- `torchFlicker`: Flicker intensity multiplier
- `ambientLight`: Base ambient brightness
- `aoLightBlend`: Blend factor for AO modulation

### 2D Screen Shader (Vertex + Fragment)

**Purpose**: Renders screen-quad overlays and upscaling

**Modes**:
1. **Texture Rendering**: Direct texture display for minimap and HUD
2. **Framebuffer Upscaling**: Scales low-res rendering to screen resolution

**Distortion Effects**:
- Optional CRT-like barrel distortion
- Aspect ratio correction
- Configurable distortion intensity

**Alpha Blending**:
- Supports transparency for minimap overlays
- Separate alpha channel for each layer

### SSAO Shader (Full-Screen Pass)

**Purpose**: Screen-space ambient occlusion with optional dithering

**SSAO Algorithm**:
- Samples depth texture to reconstruct 3D positions
- Generates random sample kernel around each pixel
- Tests occlusion of samples within local radius
- Produces smooth AO factor from 0 (fully occluded) to 1 (unoccluded)

**Input Textures**:
- Color texture: Low-res rendered scene
- Depth texture: 24-bit depth values

**Dithering Integration**:
- Optional ordered dither pattern applied to output
- Reduces apparent color banding artifacts
- Creates retro pixel-art appearance

**Output**:
- Modified color texture with AO shadows integrated

## Geometry System

### Vertex Attribute Layout

All geometry uses a consistent vertex format (Stride = 6 floats):

| Offset | Type | Purpose |
|--------|------|---------|
| 0 | 3× float | Position (X, Y, Z) |
| 3 | 2× float | Texture Coordinates (U, V) |
| 5 | 1× float | Ambient Occlusion (0.0-1.0) |

### Geometry Types

#### Wall Geometry
- **Generation**: Procedurally created based on MAP_SIZE×MAP_SIZE grid
- **Faces**: Each wall cell generates visible faces (South, North, East, West)
- **Hidden Surface Removal**: Only generates faces adjacent to walkable floor space
- **Texture**: Single wall texture mapped across all wall faces
- **AO Value**: 1.0 (no pre-baked shadows for bright, clean walls)

#### Floor/Ceiling Geometry
- **Coverage**: Spans entire MAP_SIZE×MAP_SIZE area
- **Structure**: Single mesh for all floor and ceiling surfaces
- **Height**: Floor at Y=0, ceiling at Y=1
- **Texture**: Floor/ceiling texture with consistent scale
- **AO Value**: 1.0 (no pre-baked shadows)

#### Screen Quad (Upscaling)
- **Vertices**: 6 vertices (2 triangles) covering full screen
- **Format**: Position (X, Y) + Texture coordinates (Z, U, V)
- **NDC Range**: [-1, 1] normalized device coordinates
- **Use**: Full-screen post-processing passes

### Vertex Buffer Objects (VBO)

| Buffer | Purpose | Update Frequency |
|--------|---------|------------------|
| Wall VBO | Store wall geometry | Static (generated once) |
| Floor/Ceiling VBO | Store floor/ceiling | Static (generated once) |
| Screen Quad VBO | Upscaling geometry | Static (hardcoded) |

## Texture System

### Texture Loading

**Supported Formats**: PNG via stb_image library

**Loaded Textures**:
1. **Wall Texture** (`wall.png`): Applied to all wall surfaces
2. **Floor Texture** (`floor.png`): Applied to floor and ceiling

### Texture Filtering

**Anisotropic Filtering**:
- **Support Check**: GL_EXT_texture_filter_anisotropic
- **Max Level**: Queried from GPU capabilities
- **Setting**: Configurable via `ANISOTROPY_LEVEL` (default: 8.0x)
- **Effect**: High-quality filtering at oblique angles

**Magnification Filter**: GL_NEAREST (preserves pixel-art style)
**Minification Filter**: GL_NEAREST (prevents blurring at distance)

### Texture Coordinates

- **Range**: 0.0 to 1.0 across wall/floor surfaces
- **Wrapping**: Repeating for seamless tiling
- **Scaling**: Consistent 1:1 texel-to-unit mapping

## Color & Dithering

### Color Depth

**Rendering Pipeline**:
- Internal: RGB8 (24-bit color, 8 bits per channel)
- Dithering: Optional reduction to lower bit depths
- Perception: Maintains color quality through ordered dithering

### Dithering Algorithms

**Ordered Dithering**:
- Uses predefined dither matrices for consistent patterns
- Reduces visible banding in gradients
- Creates retro pixel-art aesthetic

**Palette Support**:
- Multiple configurable dither patterns
- Selection via `DITHER_PALETTE` setting
- Creates distinct visual styles

**Bayer Matrix Example**:
```
 0  8  2 10
12  4 14  6
 3 11  1  9
15  7 13  5
```

### Dithering Application

Enabled via `USE_DITHER` setting:
- **Disabled**: Smooth color gradients
- **Enabled**: Dithered color reduction for retro feel

## Screen Resolution & Scaling

### Resolution Settings

| Component | Resolution | Purpose |
|-----------|------------|---------|
| Game Render | 320×180 | Low-res base for retro aesthetic |
| Framebuffer Size | 320×180 | SSAO and dithering pass |
| Display Output | Native (configurable) | Upscaled final image |

### Aspect Ratio Handling

- **Display**: `calculateScale()` maintains aspect ratio
- **Upscaling**: Letterboxing or stretching based on settings
- **FOV Preservation**: Aspect-aware projection matrix

### Upscaling Methods

1. **Nearest-Neighbor**: Preserves pixel boundaries (recommended for retro style)
2. **Linear**: Smoother but slightly blurry appearance

## Minimap Visual System

### Minimap Layout

**Position**: Upper-right corner of screen
- **Width**: 140 pixels (in 320×180 grid space)
- **Height**: 140 pixels
- **Margin**: 10 pixels from top-right corner

### Visual Elements

#### Explored Areas
- **Color**: Light blue (0.2, 0.5, 1.0)
- **Opacity**: Based on exploration state

#### Unexplored Areas
- **Color**: Darker gray/blue
- **Opacity**: Dimmed for distinction

#### Player Position
- **Marker**: Bright white circle
- **Size**: Scaled based on viewport size
- **Update**: Real-time following camera position

#### Walls
- **Representation**: Lines between explored and unexplored areas
- **Color**: White or light gray
- **Thickness**: 1-2 pixels

### Transparency Modes

**Normal Mode** (TAB released):
- **Alpha**: 0.75 (75% opaque)
- **Effect**: Prominent but slightly transparent overlay

**Full-Screen Mode** (TAB held):
- **Alpha**: 0.5 (50% opaque)
- **Effect**: Strategic overview with minimal obscuration

### Rendering Integration

- **Render Timing**: After 3D geometry, before SSAO pass
- **Framebuffer**: Rendered to same low-res buffer as 3D scene
- **Blending**: Alpha-blended over 3D environment
- **Depth**: No depth writes to prevent occluding 3D geometry

## Visual Effects

### Lighting Effects

**Torch Flickering**:
- Creates realistic candlelight ambiance
- Contributes to atmospheric immersion
- Framerate-independent smooth animation

**Light Falloff**:
- Smooth distance-based attenuation
- Customizable near and far distances
- Adjustable sharpness for different moods

### Post-Processing

**Screen Distortion**:
- Optional CRT-style barrel distortion
- Configurable intensity via `SCREEN_DISTORTION`
- Enhances retro monitor aesthetic

**Ambient Occlusion Blending**:
- `aoLightBlend` parameter controls shadow prominence
- Balances between clean rendering and depth perception
- Tunable for artistic preference

## Performance Considerations

### Optimization Strategies

1. **Static Geometry**: Wall/floor geometry generated once at startup
2. **Culling**: OpenGL automatically culls off-screen geometry
3. **Low Base Resolution**: 320×180 reduces pixel processing
4. **Efficient Shaders**: Minimal per-pixel calculations
5. **Single-Pass SSAO**: Avoids expensive blur post-processing

### Framerate Targets

- **Target**: 60+ FPS on modern hardware
- **Delta-Time Scaling**: Ensures consistent gameplay regardless of actual FPS
- **Optimal Range**: 60-144 FPS for smooth, responsive experience

## Configuration Parameters

| Parameter | Type | Default | Purpose |
|-----------|------|---------|---------|
| `FOV_DEGREES` | float | 60.0 | Vertical field of view angle |
| `AMBIENT_LIGHT` | float | 0.2 | Base ambient brightness (0.0-1.0) |
| `LIGHT_RADIUS_NEAR` | float | 0.5 | Minimum torch lighting distance |
| `LIGHT_RADIUS_FAR` | float | 20.0 | Maximum torch lighting distance |
| `LIGHT_SHARPNESS` | float | 2.0 | Torch falloff curve steepness |
| `LIGHT_TRANSITION_SOFTNESS` | float | 1.0 | Smooth falloff gradient |
| `AO_LIGHT_BLEND` | float | 0.5 | AO shadow prominence (0.0-1.0) |
| `USE_DITHER` | bool | true | Enable ordered dithering |
| `DITHER_PALETTE` | int | 0 | Dither pattern selection |
| `SCREEN_DISTORTION` | float | 0.1 | CRT barrel distortion intensity |
| `ANISOTROPY_LEVEL` | float | 8.0 | Texture filtering quality |

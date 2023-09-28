# nice-lights

Visualiser and controller for the Quantum Tech Club Icosahedron LED light rig.

# Build

## Requirements for all targets
- Check out from github the nice-lights repo
- Check out dear imgui repo next to nice-lights (git clone https://github.com/ocornut/imgui.git)
- CHeck out glm next to nice-lights (git clone https://github.com/g-truc/glm.git)

## Requirements for Linux
- Install the SDL2 development package (sudo apt install libsdl2-dev)
- Install the glew development package (sudo apt install libglew-dev)
- Install the cmake package (sudo apt install cmake)
- install build-essentials for g++ and make

## Requirements for Win32 using MXE cross compilation
- Install MXE from https://mxe.cc/
- build the sdl2 package "make sdl2"
- build the cmake package "make cmake"
- build the glew package "make glew"
- You may need more MXE packages to get those to build

## Requirements for native Win32 build
- Untested but expected to work, let CMake do its thing...

## Compile
- Make a directory to build in (i.e. mkdir ~/nice-lights/build)
- Cd to that dir
- Run cmake <path to CMakeLists.txt>
- (or i686-w64-mingw32.static-cmake on MXE)
- run make

# Using
- Run nice-lights
- Move and size the ImGUI windows if needed.
- Ignore "E131 Basic", only use "E131 Rig"
- Press "read mapping file"
- Press "Transmit"
- Fiddle with Style and Generic animation controls
- You should see the rig lights be the same as the visualisation
- In the Peripheral window
- Enter the correct filename for the serial device
- Press "Open file"
- Enable "Read"
- Mess around with the controller, you should see the peripheral window updating accordingly.
- Enable "Apply" to have the controller control the visualisation.

# GUI stuff

The application uses Dear ImGUI a lot. Here is the lowdown on the controls:
- There are two floating windows. Initially ImGUI puts them on top of each other
- Click and drag the caption bar and move them apart. You can also resize them as they will be a bit squished.

## Icosahedron Window

- **Draw support frame** if not enabled then only the points lights are rendered in the 3D view
- **Camera Distance** Distance of the camera from the centre of the model (also control this with the mousewheel)
- **Style** - animation style
- **Highlight edge** Sets the edge that is highlighed when the Highlight Edge style is active.
- **Generic Animation** - various sliders that do different things depending on the style selected.

### E131 Basic

This provides E131 support WITHOUT mapping.

- **Receive** whether to receiver E131 packets and draw them in the animation
- **Don't wait**  whether to wait for the universes for all the lights before rendering or draw as soon as new data arrives.
- **Destination** IP address to send E131 basic data to
- **Transmit** enable this to transmit basic E131
- **Framerate division** how often to send E131 data, i.e. for N rendered frames then one E131 frame will be sent
- **Max universe** how many universes to send.

### E131 Rig

This provides E131 support with mapping support for the icosahedron rig.

- **Read mapping file** reads the file src/edge-map.txt into the packet mapper
- **Read local mapping** file reads the file src/edge-map-local.txt into the packet mapper
- **Transmit** enables transmission of mapped E131 packets
- **Framerate division** how often to send E131 data, i.e. for N rendered frames then one E131 frame will be sent
- **Gamma** gamma correction value to scale the LED brightness values with.
- **Packet offset** offset of the start of the colour info in the E131 packet. For WLED with is 1.

## Peripheral

- **Dev file** filename of the serial device the controller is connected to
- **Open device** opens the file (or reopens it)
- **Read** whether to read data from the controller (this will update the rest of the Peripheral gui window)
- **Apply** whether to apply the read data to the visualisation. (this will cause the values in the Peripheral window to alter the values in the Icosahedron window)

# TODO
- Source has mixed spaces and tabs :-( should be 2 spaces throughout
- Commenting is missing - this was a spare time personal project!
- ~~Makefile is rubbish, should be replaced with CMake~~ 

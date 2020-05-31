# springyHexMesh

Springy Hex Mesh is intended to be a better alternative to Snappy Hex Mesh. It follows a different meshing strategy.

It never creates a castleated mesh. It starts by snapping laminar and sharp edge cusps to a grid vertex.
Then walks the sharp edges aligning and splitting cells as required.
Then walks outward from the edges aligning cell faces.

Improved orthoganality is attained by minimizing the 'energy' of simulated springs. One type of spring is rotary with it's rest position at 90 degrees to it's neighbor edges. The other type is a non-linear 
spring based on cell edge length.  

The hexahdral grid's energy is periodically minimized during the process.  

Currently the process is opaque. For debugging and feedback purposes, a graphical display is being added using Vulkan. The project is distributed as a library and the app.  

## Getting Started

The project was developed under MS Visual Studio 2019 as a Make File Project. It was also built with gcc under WSL. That environment should work best.  

If someone with a unix box wants to contribute by getting the linux environment working, I'm interested in talking.

The source code is available under GPLv3. I don't plan to, or have the resources to, pursue violators. It's yours to use. 
However, if you use it for commercial purposes I expect fare compensation for the value you received.

I am also accepting donations through [GoFundMe.com ElectroFish](https://www.gofundme.com/f/electrofish)


### Prerequisites

1 VulkanQuickStart and it's pre-requisites
2 CMake
3 A C++-17 compliant compiler. Some of the third party code incorporated required C++17.

### Installing

VulkanQuickStart is being developed using Visual Studio 2019 as a MakeFile project with Unix/Linux compatibility being tested using WSL. The foundation libraries are currently building on WSL, but not the main app.

1 Create a project root.  
2 Create a Thirdparty directory there.  
3 Follow instructions for cloning and building VulkanQuickStart
3 Clone SpringyHexMesh to the project root.   

9 Launch VS 2019 (2017 shuould work also) - get the community edition if you don't have a copy  
10 Skip the entry screen using the 'without code' option  
11 Open the project  
12 Choose makefile project and select the root CMakeLists.txt  

A fully working linux version is in the future, but for logistal reasons I don't have a linux box to develop on. Virtual Box doesn't do graphics which makes graphics work _difficult_. WSL is the current workaround.

## Running the tests

SpringyHexMesh has no automatic tests at this time.

## Deployment

It's currently built as a fully linked app. No shared libraries are required.

## Built with Visual Studio 2019 (https://visualstudio.microsoft.com/downloads/)

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Authors

**Robert R (Bob) Tipton** - btipton <-> darkskyinnovation.com
founder [Dark Sky Innovative Solutions](http://darkskyinnovation.com/index.html)

## License

This project is licensed under the GPLv3 License - see <https://www.gnu.org/licenses/> file for details


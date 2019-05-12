---
layout: default
---
![Gallery](images/gallery_high_res.png)
## depthmapX - visual and spatial network analysis software

Latest releases can be found at the [releases page](https://github.com/SpaceGroupUCL/depthmapX/releases)

### About
  depthmapX is an open-source and multi-platform spatial analysis software for spatial networks of different scales. The software was originally developed by Alasdair Turner from the Space Syntax group as Depthmap, now open-source and available as depthmapX.

### How it works
  depthmapX works at a variety of scales from buildings and small urban areas to whole cities or states. At each scale, the aim of the software is to produce a map of spatial elements and connect them via relationship (for example, intervisibility, intersection or adjacency) and then performs a graph analysis of the resulting network. The objective of the analysis is to derive variables which may have social or experiential significance.

#### Visual analysis
  At the building or small urban scale, depthmapX can be used to assess the visual accessibility of a place in a number of ways. It can produce point isovists, (polygons that represent the visually accessible area from a location) and isovist paths that show how the view changes when moving through a space.  
  
  Isovists are the core element behind Visibility Graph Analysis (VGA), acting as the joining mechanism that converts a dense grid to a graph of intervisible points (a visibility graph), which may then be analysed using various graph measures. depthmapX provides local measures of visibility such as the isovist area and perimeter but also global ones quantifying building traversal and space centrality.  
  
  VGA can also be used as the core of an agent-based analysis. In this type of analysis, a number of software agents representing pedestrians are released into the environment. Each software agent is able to access the visual accessibility information for its current location from the visibility graph, which informs its choice of next destination. 

#### Agent analysis
  depthmapX allows for simulating different types of pedestrian behaviour by providing various ways for an agent to choose where to walk, such as towards larger spaces, long lines of sight or parts of their view that are occluded. The paths of the agents and their numbers passing through gates can be counted, and compared to actual behaviour of pedestrians passing through gates.

#### Axial and segment analysis
  At the small to medium urban scale, depthmapX can be used to derive an axial map of a layout. This means it can derive a reduced straight-line network of the open space in an environment. The axial map has been the key analysis method of space syntax research for many years, but the mathematical derivation of it is novel.  
  
  The automatic derivation within depthmapX allows an objective map for researching spatial form and function. Once the map has been generated, it may be analysed using graph measures, and the measures may be transferred to gate layers in order to compare with indicators of pedestrian or social behaviour. For larger systems where the derivation algorithm becomes cumbersome, pre-drawn axial maps or downloaded maps (such as road-centre lines) may be imported.  
  
  Axial maps can also be converted to segment maps by breaking down long axial lines into a sequence of segments that lead from junction to junction. These may be analysed using a variety of techniques that accumulate depth, such as the degree of angle change from one segment to another, metric distance, or segment steps. Examples include the calculation of the number of shortest angular paths through a segment, or the average metric distance from each segment to all others.  
  
  depthmapX can import pure geometrical data such as floor plans or line-centre maps. which may then be converted into spatial networks. They can also be converted directly into existing networks such as VGA, axial maps and segment maps. The software provides an interface for exploring and modifying these networks by representing them as geometries but also by displaying the various parameters associated with them. Finally, it allows for limited extensibility and combination of the various measures by providing a simple scripting language called "salascript".

### New developments
  Recent development efforts have resulted in an alternative 'command-line' interface that allows for automating the preparation and analysis of spatial networks with minimal user interaction. With this new interface the analysis of multiple different projects can be carried out unattended and without the need to have the graphical user interface open.  
  
  In this form depthmapX may also be embedded in projects using other programming languages such as R and python, allowing them to harness the various types of analysis in more complex applications.

### Core contributors
- Alasdair Turner
- Eva Friedrich
- Tasos Varoudis
- Christian Sailer
- Petros Koutsolampros

(for an up-to-date list of contributors see [here](https://github.com/SpaceGroupUCL/depthmapX/graphs/contributors))

### How to cite
- For publications:

  depthmapX development team. (2017). depthmapX (Version 0.6.0) [Computer software]. Retrieved from https://github.com/SpaceGroupUCL/depthmapX/

- A BibTeX entry for LaTeX users:
~~~text
  @software{depthmapx,
    title = {depthmapX},
    author = {{depthmapX development team}},
    year = {2017},
    version = {0.6.0},
    url = {https://github.com/SpaceGroupUCL/depthmapX/}}
~~~

### Training
  UCL and Space Syntax Limited have developed a [Space Syntax Online Training Platform](http://otp.spacesyntax.net/) to facilitate the dissemination of Space Syntax principles, methodologies and software. The comprehensive handbook [Space Syntax Methodology](http://discovery.ucl.ac.uk/1415080/), authored by Kinda Al-Sayed et al (2014), which is used in teaching at UCL offers an introduction and step-by-step tutorials for newbies to learn the techniques and methods of Space Syntax analysis based on depthmapX.

### More information
- For news, release announcements, and relevant discussion [subscribe to the UCL Depthmap mailing list](https://www.jiscmail.ac.uk/cgi-bin/webadmin?A0=DEPTHMAP).
- For more information please check the [documentation](https://github.com/SpaceGroupUCL/depthmapX/docs/index.md) and the [wiki](https://github.com/SpaceGroupUCL/depthmapX/wiki)
- Download the [current version of depthmapX](https://github.com/SpaceGroupUCL/depthmapX/releases).
- For any issues/bugs/crashes please create [a new issue](https://github.com/SpaceGroupUCL/depthmapX/issues/new)
- The developers and users of depthmapX can also be found on matrix/riot for more direct and extended discussions in the following channels:
  - [depthmapX-users](https://riot.im/app/#/room/#depthmapX-users:matrix.org) - for general discussion, and questions about using depthmapX
  - [depthmapX-devel](https://riot.im/app/#/room/#depthmapX-devel:matrix.org) - for development discussion


depthmapX is licensed under the [GPLv3](http://www.gnu.org/licenses/gpl-3.0.html) licence. It uses [Qt5](http://www.qt.io) as UI toolkit and build system, [Catch](https://github.com/philsquared/catch) as unit testing framework and [FakeIt](https://github.com/eranpeer/FakeIt) for test mocks.



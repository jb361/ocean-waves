# Ocean Waves
Fluid simulation is an important area of research in computer graphics, given the prevalence of fluid in the natural world. This project aims to address some of the key issues involved in simulating ocean surface waves within a game. An experimental framework is developed in order to evaluate an approach by Jerry Tessendorf, which generates sinusoidal waves on a heightmap using an inverse fast Fourier transform and statistics gathered from oceanographic research. The surface is shaded by taking into account scattered light, reflection of the sky, Fresnel term and specular highlights from the sun. A final rendering of the surface is presented in juxtaposition to a photograph of a real ocean, and the various stages of the algorithm are profiled, with experiments conducted to determine the effects of wind velocity and FFT size on the simulation. Next, a comparison is drawn between two methods of computing normals – that of the FFT and Sobel filter. In conclusion, it is found that the approach provides a fairly realistic simulation of ocean waves on deep water that is suitable for use within a real-time environment such as a game.

## Screenshot
![](https://user-images.githubusercontent.com/1145329/225888229-13261c4f-a85f-4b67-80f8-1875a1221961.png)

## System Requirements
A PC with Windows Vista or above and DirectX 11 support.

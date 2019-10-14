# dominant-colors-rgb-wheel
## Find dominant colors in images with QT and OpenCV, with a nice GUI to show results on a RGB wheel
### v0 - 2019-10-10

![Screenshot - Global](screenshots/screenshot-gui.jpg?raw=true)
<br/>

## HISTORY

* v0: launch
<br/>
<br/>

## LICENSE

The present code is under GPL v3 license, that means you can do almost whatever you want with it!

I used bits of code from several sources, which are cited within the code
<br/>
<br/>

## WHY?

Lately I paid great attention to colors in my photo editing process. Delicately tuning colors can greatly improve an already good photo, to make it stunning!

I tried to find tools on the internet, but none of them had all I wanted, so I started coding once again...

I'm not an ace of C++ and QT. So, if you don't find my code pretty never mind, because it WORKS, and that's all I'm asking of it :)
<br/>
<br/>

## WITH WHAT?

Developed using:
* Linux Ubuntu	16.04
* QT Creator 3.5
* Requires these libraries:
  * QT 5
  * openCV 4.1 compiled with openCV-contribs - should work with 3.x versions without much editing

This software should also work under Microsoft Windows: if you tried it successfully please contact me, I'd like to offer compiled Windows executables
<br/>
<br/>

## HOW?

* Help is available within the GUI, with each element's tooltip. Just hover your mouse
* All is on one window, the tool is pretty simple

### IMAGE

![Screenshot - Image](screenshots/screenshot-image.jpg?raw=true)

* Just click the "Load image" button

* You have two options before loading:
	* Reduce size: the biggest the image, the longest you wait! Tests have shown that reducing the image to 512 pixels doesn't affect much the dominant colors distribution. It also helps with noisy images
	* Gaussian blur: you might not want to reduce the image, but image noise can affect results VS what you really perceive. The solution is to apply a 3x3 Gaussian blur that helps smooth surfaces

### FINDING DOMINANT COLORS

![Screenshot - Image](screenshots/screenshot-compute.jpg?raw=true)

* How many dominant colors do you want? Choose wisely, bigger values take greater time to compute

* You have to choose the algorithm first. Two are at your service:
	* Eigen vectors: the fastest, but a bit less accurate - source: http://aishack.in/tutorials/dominant-color/
	* K-means: a well-known algorithm to aggregate significant data  - source: https://jeanvitor.com/k-means-image-segmentation-opencv/

* Click Compute to finish: you end up with an updated Color Wheel, a quantized image and a palette. The elapsed time is shown in the LCD display

### COLOR WHEEL

![Screenshot - Image](screenshots/screenshot-color-wheel.jpg?raw=true)

* The Color Wheel is where the dominant colors of the image are now displayed:
	* it shows the dominant colors, so it can help you analyze how a photo or painting colors are distributed
	* you can easily understand the color combinations:
		* complementary colors are opposite on the wheel (on the same diameter)
		* analogous colors are near each other
		* triadic colors are located on an equilateral triangle which center is the wheel center
		* etc

* The Color Wheel representation is based on HSV color space:
	* Hue is represented by the angle in degrees in the circle (clockwise, where red = 0)
	* Saturation is the distance from the center
	* Value is not represented here, HSV is a 3-dimensionnal color space (a cylinder)

* Darker colors are near the center, lighter colors are near the inner circle. White, gray and black colors are centered on the wheel

* The main 12 additive colors can be found on the outer circle to help caracterizing a dominant color:
	* Primary colors (Red, Green, Blue) are in the biggest circles
	* Secondary (Cyan, Magenta, Yellow) and Tertiary colors are also represented

* The circle size of a dominant color is the percentage of this color in the whole image (in the inner circle). A circle can't be less than 40 pixels

### PALETTE

![Screenshot - Image](screenshots/screenshot-palette.jpg?raw=true)

* With the Color Wheel, another useful feature is the Palette: it show all the dominant colors, and their proportion in the Quantized image

* Speaking of it, the Quantized image shows the image transposed to the dominant colors Palette. This way you can visually check if the number of dominant colors chosen at the beginning is sufficient or too wide

* You can left-click with your mouse on any color on the wheel, or the quantized image, or even the palette, to get information:
	* RGB values of picked color, in decimal and hexadecimal
	* percentage of use in the quantized image
	* color name. Sometimes it is poetic, and sometimes it is just a code. This information was tricky to apply:
		* I used a text file containing more than 9000 RGB values and corresponding color names from http://mkweb.bcgsc.ca/colornames
		* if the exact RGB value is found the name is displayed, if not the nearest color is displayed (using the euclidian distance in RGB color space between the two colors)

* If you want to keep the results, click on the "Save Images" button on the Color Wheel. They will be saved in the same folder as the original image, with suffixes ( for palette, color wheel, quantized image)


### ACCURACY

![Screenshot - Image](screenshots/palette.png?raw=true)

How efficient are the two algorithms ? To check, I just had to test my tool on a RGB palette I produced in 5 minutes with Photoshop with the exact RGB values of the 12 Primary to Tertiary colors.

First, Eigen vectors:

![Screenshot - Image](screenshots/screenshot-accuracy-eigen.jpg?raw=true)

Then, K-means:

![Screenshot - Image](screenshots/screenshot-accuracy-k-means.jpg?raw=true)

The two methods seem pretty good at first glance. Even more when you click on the palette: a perfect match for both!

The difference between the two methods will only be seen with complex images.

<br/>
<br/>

## Enjoy!

### AbsurdePhoton
My photographer website ''Photongénique'': www.absurdephoton.fr

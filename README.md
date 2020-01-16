# dominant-colors-rgb-wheel
## Find dominant colors in images with QT and OpenCV, with a nice GUI to show results on a RGB wheel - Colors analysis includes color schemes, brightness and cool/warm distribution - All algorithms done in CIELab color space!
### v1.0 - 2020-01-11

![Screenshot - Global](screenshots/screenshot-gui.jpg?raw=true)
<br/>

## HISTORY

* v1.0: LOTS of changes, including many bug fixes:
    * All algorithms now compute in CIELab color space, using slower but precise CIEDE2000 formula to compute color distances
    * Added sophisticated image filters like "Filter grays" to only keep colors in final results, and "Regroup" to group similar scattered values
    * Added Mean-shift algorithm to quantize the image - not exactly a quantization method, but it produces similar results
    * Added the Analyze feature, which :
        * draws colors schemes over the wheel values to show for example complementary or triadic colors
        * shows brightness and cool/warm distribution
    * Save popular palette formats for Photoshop, Paintshop Pro and Corel Draw
* v0.1: found a better way to represent colors : using conversion from RGB to HSL instead of HSV, to have blacker colors near the center of the wheel and lighter at the edge of the inner circle - also added a circle size factor slider
* v0: launch
<br/>
<br/>

## LICENSE

* The present code is under GPL v3 license, that means you can do almost whatever you want with it!

* I used bits of code from several sources, which are cited within the code
<br/>
<br/>

## WHY?

* Lately I paid great attention to colors in my photo editing process. Delicately tuning colors can greatly improve an already good photo, to make it stunning!

* I wanted to analyze the color distribution and schemes of my own photos: I tried to find tools on the internet, but none of them had all I wanted, so I started coding once again...
<br/>
<br/>

## WITH WHAT?

Developed using:
* Linux Ubuntu	18.04
* QT Creator 4.5
* Requires these libraries:
    * QT 5
    * openCV 4.2 compiled with openCV-contribs - should work with 3.x versions without much editing

This software should also work under Microsoft Windows, with adjustments: if you compiled it successfully please contact me, I'd like to offer compiled Windows executables too
<br/>
<br/>

## HOW?

* Help is available within the GUI, with each element's tooltip. Just hover your mouse

* All is on one window, the tool is now more complex than the previous beta version

### IMAGE

![Screenshot - Image](screenshots/screenshot-image.jpg?raw=true)

* Just click the "Load image" button

* You have two options before loading:
	 * Reduce size: the biggest the image, the longest you wait! Tests have shown that reducing the image to 512 pixels doesn't affect much the dominant colors distribution. It also helps with noisy images
	 * Gaussian blur: you might not want to reduce the image, but image noise can affect results VS what you really perceive. The solution is to apply a 3x3 Gaussian blur that helps smooth surfaces
	 * If you want precise results, don't check any of these two options!

### FINDING DOMINANT COLORS

![Screenshot - Quantize](screenshots/screenshot-quantize.jpg?raw=true)

* How many dominant colors do you want? Choose wisely, bigger values take greater time to compute

* You have to choose the algorithm first. Three are at your service!
    * All the algorithms now compute in CIELab color space. I coded my own implementation of color conversions, because the ones from OpenCV were not accurate enough (for example loss when converting to CIE XYZ then to CIELab and back to RGB)
	 * Eigen vectors: the fastest of the three - source: http://aishack.in/tutorials/dominant-color/ - this one was trickier to adapt to work in CIELab, I also unlocked the 128 colors limit
	 * K-means: a well-known algorithm to aggregate significant data - source: https://jeanvitor.com/k-means-image-segmentation-opencv/
	 * Mean-shift: NOT exactly a quantization algorithm, but it reduces colors in an interesting way. It is also a bit destructive for the image with higher parameters values. As the number of computed colors is variable with this algorithm, when you choose the number of colors to quantize, only the N most used colors in the Quantized image are shown in the Palette

* Click "Analyze" to finish: you end up with an updated Color Wheel, a Quantized image and a Palette. The elapsed time is shown in the LCD display

![Screenshot - Quantized](screenshots/screenshot-quantized.jpg?raw=true)

* Filters:
    * "Filter grays":
        * helps filtering all near non-color values like whites, blacks and grays, to only obtain colors in the Palette and Color wheel
        * "grays" means not only gray values, because black and white are particular grays
        * the blacks, whites and grays parameters are the percentage you want to filter. This percentage is from the distance in CIELab space to the white and black points. For grays the distance is from the "black to white" grayscale
        * filtered values are shown in black color on the Quantized image
    * "Regroup" filter:
        * it helps clustering scattered similar color values
        * for example if you obtain 3 green hues near each other, it can be interesting to merge them in one big color value in the Palette
        * two parameters are used: "angle" is the Hue difference (in degrees) on the color wheel, and "distance" is the CIELab color distance between two color values
        * example without and with Regroup filter:
        
![Screenshot - Regroup off](screenshots/screenshot-regroup-off.jpg?raw=true)
![Screenshot - Regroup on](screenshots/screenshot-regroup-on.jpg?raw=true)        
        
* "Filter < x%": 
        * filter out colors representing less than x% of the image
        * it helps cleaning the Color Wheel of non-significant values

* A good overall advice: try to find the minimum number of colors that roughly represent the source image. If a major color hue is missing, try increasing the number of colors to quantize

![Screenshot - Advice 1](screenshots/screenshot-quantize-5-colors.jpg?raw=true)
![Screenshot - Advice 2](screenshots/screenshot-quantize-12-colors.jpg?raw=true)

### COLOR WHEEL

![Screenshot - Color Wheel](screenshots/screenshot-color-wheel.jpg?raw=true)

* The Color Wheel is where the dominant colors of the image are displayed after using the "Quantize" button:
    * the dominant colors are shown using color disks, the size indicating the percentage of use in the image
    * that way, you can easily understand the color schemes and distributions

* The Color Wheel representation is based on a quasi-HSL color space:
	 * Hue is represented by the angle in degrees in the circle (clockwise, where red = 0) - Hue is computed in HSL color space
	 * Saturation is not used directly here, only the colors in disks show the values
	 * Lightness is computed in CIELab color space, which is more human-perception-accurate. Darker color values are near the center of the wheel, and lighter near the inner circle. Pure white, gray and black colors are aligned by default on the red (H=0) axis

* The main 12 additive colors can be found on the outer circle, as a helper:
	 * Primary colors (Red, Green, Blue) are in the biggest circles
	 * Secondary (Cyan, Magenta, Yellow) and Tertiary colors are also represented

* You can increase or decrease the whole bunch of color disks sizes with the slider, or use the mouse wheel - useful when using higher numbers of colors to quantize

### PALETTE

![Screenshot - Palette](screenshots/screenshot-palette.jpg?raw=true)

* Another feature is the Palette: it show all the dominant colors, and their proportional percentage in the Quantized image

* Speaking of it, the Quantized image shows the image transposed to the dominant colors Palette (exact palette for Eigen and K-means algorithm). This way you can visually check if the number of dominant colors chosen at the beginning is sufficient or too wide

* You can left-click with your mouse on any color disk on the wheel, or the quantized image, or even the palette, to get information:
	 * RGB values of picked color, in decimal and hexadecimal
	 * percentage of use in the quantized image
	 * color name. Sometimes it is poetic, and sometimes it is just a code. This information was tricky to apply:
	     * I used a text file containing more than 9000 RGB values and corresponding color names from http://mkweb.bcgsc.ca/colornames
		  * if the exact RGB value is found the name is displayed, if not the nearest color is displayed (using euclidian distance in CIELab color space between the two colors)
		  * remember to put color-names.csv in the same folder as the executable (particularly in your compiling folder)

* If you want to keep the results, click on the "Save results" button on the Color Wheel. They will be saved with the provided file name + suffixes:
	* Palette: filename-palette.png
	* Color Wheel: filename-color-wheel.png
	* Quantized image: filename-quantized.png
	* CSV file of palette: filename-palette.csv - RGB values (decimal and hexadecimal) and percentage are saved
	* Several wide-used palette formats, such as Photoshop, Paintshop Pro and Corel Draw

### ANALYZE
![Screenshot - Analyze](screenshots/screenshot-analyze.jpg?raw=true)

* This is the big bonus of this version! Just click on the "Analyze" button to get interesting information on:
    * Color schemes:
        * find out how your colors are distributed, which schemes are used
        * the big buttons, when lighted, show if the image has values that are complementary, triadic, etc. The color bar indicates which colored lines are drawn on the Color wheel, which are drawn between color disks or on the outer circle
        * two options: 
            * "on borders": the lines are normally drawn between the colored disks. With this option they are drawn on the borders of the wheel
            * "only 12 hues": lines are aligned on the nearest color on the outer circle of the wheel
        * list of color schemes:
            * complementary (red lines): opposite colors, high contrast, vibrant look
            * split-complementary (cyan lines): one color opposite to two others, adjacent to the complement, same effect as complementary but a bit attenuated
            * analogous (green lines): colors next to each other. Often found in nature, harmonious and pleasing to the eye
            * triadic (blue lines): evenly spaced around the color wheel, harmonies tend to be quite vibrant
            * tetradic (rectangle, violet lines): two complementary pairs in mirror, many possibilities in variation
            * square (orange lines): similar to tetradic, but with all four colors spaced evenly around the color circle
            * monochromatic: only very close colors of almost the same hue are used, only varying in chroma and brightness    
    * Color distribution: the graph shows the colors percentages in the main 12 colors of the wheel
    * Color lightness/brightness:
        * blacks, whites and grays percentages of the image are shown
        * this uses the parameters values of the "Quantize" section, even if the option is not checked
        * the percentage of colors is also shown
        * it can help fine-tune the Quantize parameters
    * Cool/warm colors:
        * the most significant value is displayed, in percentage of the whole image
        * four categories:
            * hot: color hues between 330 and 80° on the wheel (reds and oranges and yellows)
            * warm: 80 to 150° (greens)
            * cool: 270 to 330° (violets and pinks)
            * cold: 150 to 270° (blues)
    * Perceived brightness:
        * human eyes perceive brightness of a color in a different way than pure computed values
        * the percentage shown is the global perceived brightness of the image
    * "Min. color" parameter: only consider color values representing more than x% of the Quantized image

### ACCURACY

![Screenshot - 12 colors](screenshots/screenshot-12-colors-quantized.jpg?raw=true)

* How efficient are the three algorithms?
    * To check, I just had to test my tool on a 12-colors RGB palette I produced in 5 minutes with Photoshop, with the exact RGB values of the 12 Primary to Tertiary colors
    * First result: all the algorithms produce the same exact 12-colors palette as the source (for Mean-shift with distance and color=1). A perfect match!

![Screenshot - 12 colors on wheel](screenshots/screenshot-12-colors-wheel.jpg?raw=true)    

* With more complex images, the results are a bit different. Here is an example of the same image computed with the algorithms. Order: Eigen vectors, K-Means clustering and Mean-shift clustering

![Screenshot - compare Eigen](screenshots/screenshot-compare-eigen.jpg?raw=true)
![Screenshot - compare K-means](screenshots/screenshot-compare-k-means.jpg?raw=true)
![Screenshot - compare Mean-shift](screenshots/screenshot-compare-mean-shift.jpg?raw=true)

Notice that K-means result is in fact the average of 100 runs of the formula, intialized with pseudo-random values. That means each time you run the algorithm, you get a bit different result.

<br/>
<br/>

## Enjoy!

### AbsurdePhoton
My photographer website ''Photongénique'': www.absurdephoton.fr

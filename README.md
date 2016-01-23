# Graph128 - Micro Graphing Calculator

The Graph128 is a super-lightweight and flexible graphing calculator designed for the ST7735 128x160 LCD display.  

#### Features

* It's easy to use - just use the joystick to move the cursor, and click with the button

* It's customizable - four unique colorschemes *just for you*

* It'll handle any input function you can think of*

<sup>* see section "Wishful Thinking" below</sup>

![graph screen](/graph_screen.jpg?raw=true)

[Watch the video to see it in action.](https://www.youtube.com/watch?v=kQ2EMQ__5mo)

#### Technical overview

- 1x 128x160 ST7735 TFT LCD display (using Adafruit breakout board)
- 2x ATmega1284 processors
- MCU0 manages:
  - Calculator functions (such as assembling functions, conversion to RPN, RPN evaluation, and graphing)
  - Bare-metal GUI featuring cursor movement, highlight effects, and event handling
  - From-scratch ST7735 driver featuring fast line & text rasterization
- MCU1 manages:
  - User input and pattern recognition
  - Sending control signals via USART to MCU0
- 1x 2-axis joystick
- 1x cheap pushbutton

##### To start with a demo:
 
- Power on and a menu with the display “GRAPH128” has (hopefully) appeared.

- Move the cursor down, and select “Load Graph”.

- Navigate to any of the numbered equations and select it with the cursor.

- This will open up the function editor.  Navigate to the gigantic “OK” button in the corner.

- Look at graphs!  Fun!

##### To create an equation from scratch:
  - Follow the first step above. (psst: turn it on)
  - Select “Create Graph” in the main menu.
  - Click the elements you want to add to the equation.

    - If you make a mistake, the “del” button will delete ONE character at a time from the end (like a backspace)
    - If you want to nuke it, click the “clear” button.
    - If you’re lazy, click the “load” button to view the function demo page.
    
  - When you’re satisfied, click the big honking “OK” button in the corner to graph!

After the function has been plotted successfully, you can press “back” to return to the function editor to make changes if you wish.  “main” brings you back to the main menu.

##### To change settings
  - Navigate to the “Settings” button in the main menu.
  - Here you will see twooptions:
    - “Toggle colorscheme”
changes the color-scheme of the entire calculator.  
Generally speaking, it swaps between light letters on a dark background, and dark letters on a light background.
    - “Invert colors”
performs a hardware color inversion - very cheap!  
You can use this to tweak the existing colorschemes to your liking.


#### Additional notes:
* Any errors in the equation syntax will result in a fatal error, requiring a restart of the system. This is a design decision :)

* Touching exposed leads, especially around the joystick, may cause accidental movement of the cursor.

* The Atmegas are not linked with SPI, so it is safe to use an ISP on each one without interfering with the other.


##### Wishful thinking

* Modifiable graph scale.  For now it only displays a window from -6 to 6 on both axes.

* Persistent memory slots

* Expanded function input as well as support for longer functions (for now they are limited only by how much fits on the screen)


P.S. This project was mostly coded from the bottom up over the course of 2 days.
So, if you find any problems or have complaints...
Feel free to post an issue!

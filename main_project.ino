#include <avr/pgmspace.h>
#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

#define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
#define OE 9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
// Last parameter = 'true' enables double-buffering, for flicker-free,
// buttery smooth animation.  Note that NOTHING WILL SHOW ON THE DISPLAY
// until the first call to swapBuffers().  This is normal.
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);

// BEGIN CONSTRUCTING ALL VARIABLES AND OBJECTS USED IN THIS PROJECT

// player object
struct player {
  uint8_t xpos;   // x-coordinate of player
  uint8_t ypos;   // y-coordinate of player
};

player player;

// stairs object
struct stairs {
  uint8_t xpos;   // x-coordinate of stairs
  uint8_t ypos;   // y-coordinate of stairs
};

stairs stairs;

// key object
struct key {
  uint8_t xpos;   // x-coordinate of key
  uint8_t ypos;   // y-coordinate of key
};

key key1, key2, key3, key4;

uint8_t temp = 0x00;        // data received from ATmega1284
uint8_t data = 0x00;        // data translated from ATmega1284
uint8_t task = 0x00;        // acts as identifier to which task the ArduinoUno should do
uint8_t action = 0x00;      // acts as identifier to which button the player has pressed
uint8_t button = 0;         // determines if a button is pressed down or not
uint8_t level = 1;          // determines which level the player is entering

uint8_t clear_start = 0;    // variable to clear start screen once
uint8_t clear_play = 0;     // variable to clear level screen once
uint8_t clear_level1 = 0;   // variable to clear level 1 display screen once
uint8_t clear_level2 = 0;   // variable to clear level 2 display screen once
uint8_t clear_level3 = 0;   // variable to clear level 3 display screen once
uint8_t clear_end = 0;

// END CONSTRUCTING ALL VARIABLES AND OBJECTS USED IN THIS PROJECT

// BEGIN HELPER FUNCTIONS TO RETURN CERTAIN COLORS

// player color
int pColor() {
  return matrix.Color333(5, 3, 0);
}

// stairs color
int sColor() { // Stair color
  return matrix.Color333(7, 0, 7);
}

// key color
int kColor() {
  return matrix.Color333(2, 2, 2);
}

// level 1 (map1) color
int color1() { // Color for map 1 (LEVEL 1 - blue)
  return matrix.Color333(0, 0, 7);
}

// level 2 (map2) color
int color2() { // Color for map 2 (LEVEL 2 - green)
  return matrix.Color333(0, 7, 0);
}


// level 3 (map3) color
int color3() { // Color for map 3 (LEVEL 3 - red)
  return matrix.Color333(7, 0, 0);
}

// null color (black)
int nColor() {
  return matrix.Color333(0, 0, 0);
}

// END HELPER FUNCTIONS TO RETURN CERTAIN COLORS

// Runs once for setup
void setup() {
  matrix.begin();
  Serial.begin(9600);

  // The first task should always be the start screen.
  task = 0x20;
}

// Helper function for player invalid movements.
void stay() {
  matrix.drawPixel(player.xpos, player.ypos, pColor());
}

// BEGIN MAPS

// MAP1
void map1() {
  matrix.drawRect(8, 4, 16, 8, color1());
  matrix.drawLine(9, 7, 15, 7, color1());
  matrix.drawLine(19, 5, 19, 8, color1());
  
  player.xpos = 10;
  player.ypos = 9;
  matrix.drawPixel(10, 9, pColor());
  
  stairs.xpos = 21;
  stairs.ypos = 6;
  matrix.drawPixel(21, 6, sColor());

  key1.xpos = 10;
  key1.ypos = 6;
  //matrix.drawPixel(10, 6, kColor());
}

// MAP2
void map2() {
  matrix.drawRect(8, 4, 16, 8, color2());
  matrix.drawLine(12, 5, 12, 8, color2());
  matrix.drawLine(19, 5, 19, 8, color2());
  matrix.drawLine(13, 8, 16, 8, color2());

  player.xpos = 21;
  player.ypos = 6;
  matrix.drawPixel(21, 6, pColor());

  stairs.xpos = 10;
  stairs.ypos = 6;
  matrix.drawPixel(10, 6, sColor());

  key1.xpos = 12;
  key1.ypos = 10;
  //matrix.drawPixel(12, 10, kColor());

  key2.xpos = 14;
  key2.ypos = 6;
  //matrix.drawPixel(14, 6, kColor());
}

// MAP3
void map3() {
  matrix.drawRect(8, 4, 16, 8, color3());
  matrix.drawLine(12, 6, 22, 6, color3());
  matrix.drawLine(12, 8, 12, 10, color3());
  matrix.drawLine(16, 8, 16, 10, color3());
  matrix.drawLine(20, 8, 20, 10, color3());

  player.xpos = 10;
  player.ypos = 6;
  //matrix.drawPixel(10, 6, pColor());

  stairs.xpos = 18;
  stairs.ypos = 10;
  matrix.drawPixel(18, 10, sColor());

  key1.xpos = 22;
  key1.ypos = 5;
  //matrix.drawPixel(22, 5, kColor());

  key2.xpos = 22;
  key2.ypos = 10;
  //matrix.drawPixel(22, 10, kColor());

  key3.xpos = 14;
  key3.ypos = 10;
  //matrix.drawPixel(14, 10, kColor());

  key4.xpos = 10;
  key4.ypos = 10;
  //matrix.drawPixel(10, 10, kColor());
}

// END MAPS

// BEGIN PLAYER MOVEMENT

void goUp() {
  player.ypos--;
  matrix.drawPixel(player.xpos, player.ypos, pColor());
}

void goDown() {
  player.ypos++;
  matrix.drawPixel(player.xpos, player.ypos, pColor());
}

void goLeft() {
  player.xpos--;
  matrix.drawPixel(player.xpos, player.ypos, pColor());
}

void goRight() {
  player.xpos++;
  matrix.drawPixel(player.xpos, player.ypos, pColor());
}

// END PLAYER MOVEMENT

void play() {

  if(!clear_play) {
    matrix.fillScreen(nColor());
    clear_play = 1;
    if(level == 1) {
      map1();
    } else if(level == 2) {
      map2();
    } else if(level == 3) {
      map3();
    }
  }

  //Setting previous pixel to black
  if(action != 0x00) {
    if(!button) {
      matrix.drawPixel(player.xpos, player.ypos, nColor());   
    }
  }

  //Nothing is being pressed so button is set to 0
  if(action == 0x00) {
    button = 0;
    matrix.drawPixel(player.xpos, player.ypos, pColor());
  }

  //UP
  if(action == 0x01) {
    if(player.ypos == 5) {
      stay();
    } else if(!button) {
      if(level == 1) {
        if(player.xpos < 16 && player.ypos == 8) {
          stay();
        } else if(player.xpos == 19 && player.ypos == 9) {
          stay();
        } else {
          goUp();
        }
      } else if(level == 2) {
        if((player.xpos == 19 || (player.xpos > 11 && player.xpos < 17)) && player.ypos == 9) {
          stay();
        } else {
          goUp();
        }
      } else if(level == 3) {
        if((player.xpos > 11 && player.xpos < 23) && player.ypos == 7) {
          stay();
        } else {
          goUp();
        }
      }
      button = 1;
    }
  }

  //DOWN
  else if(action == 0x02) {
    if(player.ypos == 10) {
      stay();
    } else if(!button) {
      if(level == 1) {
        if(player.xpos < 16 && player.ypos == 6) {
          stay();
        } else {
          goDown();
        }
      } else if(level == 2) {
        if((player.xpos > 12 && player.xpos < 17) && player.ypos == 7) {
          stay();
        } else {
          goDown();
        }
      } else if(level == 3) {
        if((player.xpos > 11 && player.xpos < 23) && player.ypos == 5) {
          stay();
        } else if((player.xpos == 12 || player.xpos == 16 || player.xpos == 20) && player.ypos == 7) {
          stay();
        } else {
          goDown();
        }
      }
      button = 1;
    }
  }

  //LEFT
  else if(action == 0x03) {
    if(player.xpos == 9) {
      stay();
    } else if(!button) {
      if(level == 1) {
        if(player.xpos == 16 && player.ypos == 7) {
          stay();
        } else if(player.xpos == 20 && player.ypos < 9) {
          stay();
        } else {
          goLeft();
        }
      } else if(level == 2) {
        if((player.xpos == 13 || player.xpos == 20) && player.ypos < 9) {
          stay();
        } else if(player.xpos == 17 && player.ypos == 8) {
          stay();
        } else {
          goLeft();
        }
      } else if(level == 3) {
        if((player.xpos == 13 || player.xpos == 17 || player.xpos == 21) && (player.ypos > 7 && player.ypos < 11)) {
          stay();
        } else {
          goLeft();
        }
      }
      button = 1;
    }
  }

  //RIGHT
  else if(action == 0x04) {
    if(player.xpos == 22) {
      stay();
    } else if(!button) {
      if(level == 1) {
        if(player.xpos == 18 && player.ypos < 9) {
          stay();    
        } else {
          goRight();
        }
      } else if(level == 2) {
        if((player.xpos == 11 || player.xpos == 18) && player.ypos < 9) {
          stay();
        } else {
          goRight();
        }
      } else if(level == 3) {
        if((player.xpos == 11 || player.xpos == 15 || player.xpos == 19) && (player.ypos > 7 && player.ypos < 11)) {
          stay();
        } else if(player.xpos == 11 && player.ypos == 6) {
          stay();
        } else{
          goRight();
        }
      }
      button = 1;
    }
  }

  //SELECT
  else if(action == 0x05) {
    reset();
    Serial.write(0x55);
  }
}

void reset() {
  matrix.fillScreen(nColor());

  // resets clear variables
  clear_start = 0;
  clear_play = 0;
  clear_level1 = 0;
  clear_level2 = 0;
  clear_level3 = 0;
  clear_end = 0;

  // resets level to 1
  level = 1;
}

// Displays "PRESS START" at beginning of game
void game_start() {
  if(!clear_start) {
    matrix.fillScreen(nColor());
    clear_start = 1;
  }

  matrix.setTextSize(1);
  matrix.setTextColor(matrix.Color333(7, 7, 7));
  matrix.setCursor(1, 0);
  matrix.print("PRESS");
  matrix.setCursor(1, 9);
  matrix.print("START");
  
  reset();
}

// Displays "LEVEL ONE " at beginning of level 1
void display_level1() {
  
  // clears screen once to display properly
  // otherwise will continue to flash due to looping
  if(!clear_level1) {
    matrix.fillScreen(nColor());
    clear_level1 = 1;
  }

  // sets text size, color and cursor location
  // before printing appropriate level
  matrix.setTextSize(1);    // 8 bits high
  matrix.setTextColor(matrix.Color333(7, 7, 7));    // white
  matrix.setCursor(1, 0);
  matrix.print("LEVEL");
  matrix.setCursor(1, 9);
  matrix.print(" ONE ");
  
  clear_play = 0;

  delay(500);
}


// Displays "LEVEL TWO " at beginning of level 2
void display_level2() {
  
  // clears screen once to display properly
  // otherwise will continue to flash due to looping
  if(!clear_level2) {
    matrix.fillScreen(nColor());
    clear_level2 = 1;
  }

  // sets text size, color and cursor location
  // before printing appropriate level
  matrix.setTextSize(1);    // 8 bits high
  matrix.setTextColor(matrix.Color333(7, 7, 7));    // white
  matrix.setCursor(1, 0);
  matrix.print("LEVEL");
  matrix.setCursor(1, 9);
  matrix.print(" TWO ");
  
  clear_play = 0;

  delay(500);
}

// Displays "LEVEL THREE" at begining of level 3
void display_level3() {
  
  // clears screen once to display properly
  // otherwise will continue to flash due to looping
  if(!clear_level3) {
    matrix.fillScreen(nColor());
    clear_level3 = 1;
  }

  // sets text size, color and cursor location
  // before printing appropriate level
  matrix.setTextSize(1);    // 8 bits high
  matrix.setTextColor(matrix.Color333(7, 7, 7));    // white
  matrix.setCursor(1, 0);
  matrix.print("LEVEL");
  matrix.setCursor(1, 9);
  matrix.print("THREE");
  
  clear_play = 0;

  delay(500);
}

void game_over() {
  if(!clear_end) {
    matrix.fillScreen(nColor());
    clear_end = 1;
  }

  matrix.setTextSize(1);
  matrix.setTextColor(matrix.Color333(7, 7, 7));
  matrix.setCursor(1, 0);
  matrix.print(" THE ");
  matrix.setCursor(1, 9);
  matrix.print(" END ");
}

// determines the level being played
void level_select() {
  if(player.xpos == stairs.xpos && player.ypos == stairs.ypos && action == 0x08) {
    clear_play = 0;
    level++;
    if(level == 2) {
      Serial.write(0xA2);
    } else if(level == 3) {
      Serial.write(0xA3);
    } else if(level == 4) {
      Serial.write(0xA4);
    }
  }
}

// Loops continuously
void loop() {
  if(Serial.available() > 0) {
    temp = Serial.read();
    data = (temp & 0xF0);
    Serial.println(temp);
  }

  // data type is what button is pressed
  if(data == 0x00) {
    action = temp;
  }

  // data type is what task has been sent
  if(data == 0x20) {
    task = temp;
  }

  if(task == 0x20) {
    game_start();
  } else if(task == 0x21) {
    display_level1();
  } else if(task == 0x22) {
    display_level2();
  } else if(task == 0x23) {
    display_level3();
  } else if(task == 0x24) {
    level = 1;
    play();
    level_select();
  } else if(task == 0x25) {
    level = 2;
    play();
    level_select();
  } else if(task == 0x26) {
    level = 3;
    play();
    level_select();
  } else if(task == 0x27) {
    game_over();
  }
}

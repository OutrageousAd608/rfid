/**
  ******************************************************************************
  * @file    ui.h
  * @brief   Header for User Interface logic.
  * Defines states, structures, and shared data.
  ******************************************************************************
  */

#ifndef UI_H
#define UI_H

#include "main.h"
#include "touch.h"
#include "ili9341.h"
#include "fonts.h"

// --- THEME COLORS (High Contrast Hacker Theme) ---
#define COLOR_TERM_BG    BLACK
#define COLOR_TERM_TEXT  0xFFFF // White
#define COLOR_TERM_DIM   0x07E0 // Neon Green
#define COLOR_ALERT      0xF800 // Red

// --- APPLICATION STATES ---
typedef enum {
    PAGE_BOOT,            // Startup animation
    PAGE_MAIN,            // Root menu
    PAGE_TX_LIST,         // Saved signals list
    PAGE_OPTIONS,         // Signal details (Tx/Rename/Delete)
    PAGE_CONFIRM_DELETE,  // Safety check
    PAGE_TRANSMITTING,    // Active output
    PAGE_RX_SENSING,      // Active sniffing
    PAGE_KEYBOARD         // Text entry
} AppState;

extern AppState currentState;
extern uint8_t ui_needs_update;

// --- DATABASE CONFIG ---
#define MAX_SLOTS 15      // Max signals stored
#define NAME_LEN  10      // Max chars per name
#define MAX_SIG_LEN 512   // Max edges per signal

// --- SIGNAL STRUCTURE ---
typedef struct {
    char name[NAME_LEN + 1]; 
    uint8_t is_active;       // 1 = Occupied, 0 = Empty
    uint16_t length;         // Actual number of edges recorded
    uint32_t timings[MAX_SIG_LEN]; // The actual waveform data
} Signal;

// Shared Global Data
extern Signal signal_db[MAX_SLOTS];
extern int8_t selected_slot_idx; 

// --- PUBLIC FUNCTIONS ---

/**
 * @brief  Initializes UI, Display, and Loads Data.
 */
void UI_Init(void);

/**
 * @brief  Runs the startup boot sequence animation.
 */
void UI_Draw_Boot_Sequence(void);

/**
 * @brief  Main UI Loop: Redraws the screen based on current state.
 */
void UI_Refresh(void);

/**
 * @brief  Handles touch inputs and state transitions.
 * @param  x, y: Touch coordinates.
 */
void UI_Handle_Touch(uint16_t x, uint16_t y);

/**
 * @brief  Updates animations (cursors, hex dumps) without clearing the screen.
 */
void UI_Update_Dynamic_Elements(void); 

#endif // UI_H
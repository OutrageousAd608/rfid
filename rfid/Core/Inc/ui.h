#ifndef UI_H
#define UI_H

#include "main.h"
#include "touch.h"
#include "ili9341.h"
#include "fonts.h"

// --- THEME COLORS ---
#define COLOR_TERM_BG    BLACK
#define COLOR_TERM_TEXT  0xFFFF 
#define COLOR_TERM_DIM   0x07E0 
#define COLOR_ALERT      0xF800 

// --- APPLICATION STATES ---
typedef enum {
    PAGE_BOOT,            
    PAGE_MAIN,            
    PAGE_LIST,            
    PAGE_OPTIONS,         
    PAGE_CONFIRM_DELETE,  
    PAGE_EMULATING,       
    PAGE_READING,         
    PAGE_KEYBOARD         
} AppState;

extern AppState currentState;
extern uint8_t ui_needs_update;

// --- DATABASE CONFIG ---
#define MAX_SLOTS 10      
#define NAME_LEN  10      
#define MAX_SAMPLES 2048  

typedef struct {
    char name[NAME_LEN + 1]; 
    uint8_t is_active;           
    uint16_t length;             
    uint32_t raw_data[MAX_SAMPLES]; 
} Signal;

extern Signal signal_db[MAX_SLOTS];
extern int8_t selected_slot_idx; 

// --- PUBLIC UI FUNCTIONS ---
void UI_Init(void);
void UI_Draw_Boot_Sequence(void);
void UI_Refresh(void);
void UI_Handle_Touch(uint16_t x, uint16_t y);
void UI_Update_Dynamic_Elements(void); 

// --- RFID HARDWARE STUBS (Placeholders for your new code) ---
void RFID_Init(void);
void RFID_Read_Start(void);
void RFID_Read_Stop(void);
void RFID_Carrier_Off(void);
void RFID_Emulate_Raw(volatile uint32_t *timings, uint16_t length);
uint8_t RFID_Process(void);

#endif
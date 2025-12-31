/**
  ******************************************************************************
  * @file    ui.c
  * @brief   User Interface Logic for STM32 Flipper-style RF Tool.
  * Handles state machine, drawing, touch inputs, and virtual keyboard.
  ******************************************************************************
  */

#include "ui.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 

// --- STATE MANAGEMENT ---
AppState currentState = PAGE_BOOT;
uint8_t ui_needs_update = 1;

// --- PAGINATION ---
int8_t list_page = 0; 
#define SLOTS_PER_PAGE 3

// --- DATABASE ---
Signal signal_db[MAX_SLOTS];
int8_t selected_slot_idx = -1; 

// --- KEYBOARD BUFFER ---
char input_buffer[NAME_LEN + 1];
uint8_t kb_mode = 0;  // 0 = Letters, 1 = Numbers/Symbols
uint8_t kb_shift = 0; // 0 = Lowercase, 1 = Uppercase

// --- BUTTON DEFINITIONS ---
// Main Menu
ButtonDef btn_Tx     = {10, 80,  220, 50}; 
ButtonDef btn_Rx     = {10, 150, 220, 50};

// List Slots
ButtonDef btn_Slot1  = {5, 40,  230, 40};
ButtonDef btn_Slot2  = {5, 90,  230, 40};
ButtonDef btn_Slot3  = {5, 140, 230, 40};

// Navigation
ButtonDef btn_Prev   = {5,   200, 60, 40}; 
ButtonDef btn_Next   = {175, 200, 60, 40}; 
ButtonDef btn_Back   = {60,  260, 120, 40};

// Options Page
ButtonDef btn_Opt_Tx     = {20, 75,  200, 50};
ButtonDef btn_Opt_Rename = {20, 145, 90,  40};
ButtonDef btn_Opt_Del    = {130, 145, 90, 40};
ButtonDef btn_Opt_Back   = {60,  215, 120, 40};

// Active Page Controls
ButtonDef btn_Stop       = {20, 200, 200, 60}; 

// Confirmation Page
ButtonDef btn_Conf_No    = {20, 130, 90, 60};  // Red (Left)
ButtonDef btn_Conf_Yes   = {130, 130, 90, 60}; // Green (Right)

// Keyboard Buttons (5-Key Bottom Row)
ButtonDef btn_Kb_Mode  = {2,   275, 45, 40}; // [123]
ButtonDef btn_Kb_Shift = {50,  275, 45, 40}; // [SHF]
ButtonDef btn_Kb_Space = {98,  275, 45, 40}; // [_]
ButtonDef btn_Kb_Del   = {146, 275, 45, 40}; // [DEL]
ButtonDef btn_Kb_Done  = {194, 275, 44, 40}; // [OK]

// --- PRIVATE HELPERS ---

/* Draw a hollow rectangle (Wireframe look) */
static void UI_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    LCD_FillRect(x, y, w, 1, color);       // Top
    LCD_FillRect(x, y + h - 1, w, 1, color); // Bottom
    LCD_FillRect(x, y, 1, h, color);       // Left
    LCD_FillRect(x + w - 1, y, 1, h, color); // Right
}

/* Draws a styled "Hacker Terminal" button */
static void Draw_Terminal_Button_State(ButtonDef *btn, const char* text, uint8_t is_alert, uint8_t is_active) {
    uint16_t color = is_alert ? COLOR_ALERT : COLOR_TERM_DIM; 
    uint16_t text_color = is_alert ? COLOR_ALERT : COLOR_TERM_TEXT; 
    uint16_t bg_color = COLOR_TERM_BG;

    if (is_active) {
        // Active/Toggled State
        LCD_FillRect(btn->x, btn->y, btn->width, btn->height, color);
        text_color = BLACK;
        bg_color = color;
    } else {
        // Normal State
        UI_DrawRect(btn->x, btn->y, btn->width, btn->height, color);
        // Corner Glitch Accents
        LCD_FillRect(btn->x, btn->y, 5, 5, color);
        LCD_FillRect(btn->x + btn->width - 5, btn->y + btn->height - 5, 5, 5, color);
    }

    char buf[30];
    if (strlen(text) > 0) sprintf(buf, "%s", text);
    else sprintf(buf, "---"); 

    // Center Text
    uint16_t text_len = strlen(buf) * 7;
    uint16_t x_pos = btn->x + (btn->width - text_len) / 2;
    uint16_t y_pos = btn->y + (btn->height - 10) / 2;
    LCD_WriteString(buf, x_pos, y_pos, Font_7x10, text_color, bg_color);
}

// Wrapper for standard buttons
static void Draw_Terminal_Button(ButtonDef *btn, const char* text, uint8_t is_alert) {
    Draw_Terminal_Button_State(btn, text, is_alert, 0);
}

/* Button Flash Animation */
static void Flash_Button(ButtonDef *btn, const char* restore_text, uint8_t is_alert) {
    uint16_t flash_color = is_alert ? COLOR_ALERT : COLOR_TERM_DIM;
    
    // 1. Fill Solid (Flash)
    LCD_FillRect(btn->x, btn->y, btn->width, btn->height, flash_color);
    HAL_Delay(50);
    
    // 2. Restore logic
    LCD_FillRect(btn->x + 1, btn->y + 1, btn->width - 2, btn->height - 2, BLACK);
    
    if (restore_text) {
         // Special handling for SHIFT button restoration state
         uint8_t active_state = 0;
         if (btn == &btn_Kb_Shift && kb_shift) active_state = 1;
         
         Draw_Terminal_Button_State(btn, restore_text, is_alert, active_state); 
    }
}

// --- KEYBOARD LOGIC ---

const char* kb_rows_lower[] = {"qwert", "yuiop", "asdfg", "hjklc", "zvbnm"};
const char* kb_rows_upper[] = {"QWERT", "YUIOP", "ASDFG", "HJKLC", "ZVBNM"};
const char* kb_rows_num[]   = {"12345", "67890", "-+=@#", "$%&()", "!?:;/"};

static void Draw_Keyboard_Static(void) {
    LCD_WriteString("ENTER NAME:", 10, 10, Font_7x10, COLOR_TERM_DIM, BLACK);
    UI_DrawRect(10, 25, 220, 30, COLOR_TERM_TEXT);
    
    uint16_t start_y = 65;
    uint16_t btn_w = 40;
    uint16_t btn_h = 35;
    uint16_t gap = 5;

    // Determine current layout
    const char** current_rows;
    if (kb_mode == 1) current_rows = kb_rows_num;
    else if (kb_shift == 1) current_rows = kb_rows_upper;
    else current_rows = kb_rows_lower;

    // Draw Grid Keys
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            char key_char = current_rows[row][col];
            uint16_t x = 10 + (col * (btn_w + gap));
            uint16_t y = start_y + (row * (btn_h + gap));
            
            UI_DrawRect(x, y, btn_w, btn_h, COLOR_TERM_DIM);
            char s[2] = {key_char, '\0'}; 
            LCD_WriteString(s, x + 15, y + 10, Font_7x10, COLOR_TERM_TEXT, BLACK);
        }
    }
    
    // Draw Function Keys
    Draw_Terminal_Button(&btn_Kb_Mode, (kb_mode==0)?"123":"ABC", 0);
    Draw_Terminal_Button_State(&btn_Kb_Shift, "SHF", 0, kb_shift); // Show Active state
    Draw_Terminal_Button(&btn_Kb_Space, "_", 0);
    Draw_Terminal_Button(&btn_Kb_Del, "DEL", 1);  // Red Alert
    Draw_Terminal_Button(&btn_Kb_Done, "OK", 0);
}

static void Update_Input_Display(void) {
    // Only redraw the text area, not the whole screen
    LCD_FillRect(12, 27, 216, 26, BLACK);
    LCD_WriteString(input_buffer, 15, 35, Font_7x10, COLOR_TERM_TEXT, BLACK);
    
    // Draw Cursor
    uint16_t cursor_x = 15 + (strlen(input_buffer) * 7);
    LCD_FillRect(cursor_x, 35, 7, 10, COLOR_TERM_TEXT);
}

static char Check_Keyboard_Touch(uint16_t x, uint16_t y) {
    uint16_t start_y = 65;
    uint16_t btn_w = 40;
    uint16_t btn_h = 35;
    uint16_t gap = 5;

    const char** current_rows;
    if (kb_mode == 1) current_rows = kb_rows_num;
    else if (kb_shift == 1) current_rows = kb_rows_upper;
    else current_rows = kb_rows_lower;

    // Check Grid Presses
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            uint16_t key_x = 10 + (col * (btn_w + gap));
            uint16_t key_y = start_y + (row * (btn_h + gap));
            
            if (x >= key_x && x <= key_x + btn_w && y >= key_y && y <= key_y + btn_h) {
                // Visual Flash
                LCD_FillRect(key_x, key_y, btn_w, btn_h, COLOR_TERM_DIM);
                HAL_Delay(50);
                
                // Restore
                LCD_FillRect(key_x + 1, key_y + 1, btn_w - 2, btn_h - 2, BLACK);
                char s[2] = {current_rows[row][col], '\0'}; 
                LCD_WriteString(s, key_x + 15, key_y + 10, Font_7x10, COLOR_TERM_TEXT, BLACK);
                
                return current_rows[row][col];
            }
        }
    }
    return 0;
}

// --- DATABASE HELPERS ---

int Find_Free_Slot(void) {
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (signal_db[i].is_active == 0) return i;
    }
    return -1;
}

int Get_Total_Active_Signals(void) {
    int count = 0;
    for(int i=0; i<MAX_SLOTS; i++) {
        if(signal_db[i].is_active) count++;
    }
    return count;
}

void Delete_Signal(int idx) {
    if (idx < 0 || idx >= MAX_SLOTS) return;
    
    // Shift remaining signals up to close the gap
    for (int i = idx; i < MAX_SLOTS - 1; i++) {
        signal_db[i] = signal_db[i+1];
    }
    // Clear the last slot
    signal_db[MAX_SLOTS-1].is_active = 0;
    memset(signal_db[MAX_SLOTS-1].name, 0, NAME_LEN);
    
    // Save changes to Flash
    Storage_SaveSignals();
}

// --- PUBLIC UI FUNCTIONS ---

void UI_Init(void) {
    LCD_Init();
    LCD_FillColor(COLOR_TERM_BG);
    currentState = PAGE_BOOT;
    Storage_LoadSignals(); // Load persistent data
}

void UI_Draw_Boot_Sequence(void) {
    if (currentState != PAGE_BOOT) return;
    
    LCD_FillColor(BLACK);
    HAL_Delay(500);
    LCD_WriteString("SYSTEM READY", 70, 140, Font_7x10, COLOR_TERM_TEXT, BLACK);
    HAL_Delay(500);
    
    currentState = PAGE_MAIN;
    ui_needs_update = 1;
}

void UI_Refresh(void) {
    // Avoid full redraws if not needed
    if (!ui_needs_update && currentState != PAGE_KEYBOARD) return; 
    if (currentState == PAGE_KEYBOARD && !ui_needs_update) return;

    LCD_FillColor(COLOR_TERM_BG); 
    char slot_buf[30];

    switch (currentState) {
        case PAGE_BOOT: break;

        case PAGE_MAIN:
            LCD_WriteString("// ROOT_ACCESS", 5, 10, Font_7x10, COLOR_TERM_DIM, BLACK);
            LCD_FillRect(0, 25, 240, 1, COLOR_TERM_DIM);
            Draw_Terminal_Button(&btn_Tx, "> EXECUTE_PAYLOAD", 0);
            Draw_Terminal_Button(&btn_Rx, "> SNIFF_TRAFFIC", 0);
            break;

        case PAGE_TX_LIST:
        {
            char title[30];
            sprintf(title, "// LIST [PG %d]", list_page + 1);
            LCD_WriteString(title, 5, 10, Font_7x10, COLOR_TERM_DIM, BLACK);
            LCD_FillRect(0, 25, 240, 1, COLOR_TERM_DIM);
            
            int start_idx = list_page * SLOTS_PER_PAGE;
            int total_signals = Get_Total_Active_Signals();
            
            // Draw Slots
            for (int i = 0; i < 3; i++) {
                int db_idx = start_idx + i;
                ButtonDef* btn = (i==0) ? &btn_Slot1 : (i==1) ? &btn_Slot2 : &btn_Slot3;
                
                if (db_idx < MAX_SLOTS && signal_db[db_idx].is_active) {
                    sprintf(slot_buf, "> %s", signal_db[db_idx].name);
                    Draw_Terminal_Button(btn, slot_buf, 0);
                } else {
                    Draw_Terminal_Button(btn, "", 0);
                }
            }
            
            // Draw Navigation
            if (list_page > 0) Draw_Terminal_Button(&btn_Prev, "< PREV", 0);
            if (total_signals > (start_idx + SLOTS_PER_PAGE)) Draw_Terminal_Button(&btn_Next, "NEXT >", 0);
            Draw_Terminal_Button(&btn_Back, "< HOME", 1); 
            break;
        }

        case PAGE_OPTIONS:
        {
            LCD_WriteString("// SIGNAL OPT.", 5, 10, Font_7x10, COLOR_TERM_DIM, BLACK);
            LCD_FillRect(0, 25, 240, 1, COLOR_TERM_DIM);
            
            LCD_WriteString("SELECTED:", 88, 35, Font_7x10, COLOR_TERM_DIM, BLACK);
            
            // Centered Name
            char* name = signal_db[selected_slot_idx].name;
            int name_width = strlen(name) * 7;
            int name_x = (240 - name_width) / 2;
            LCD_WriteString(name, name_x, 55, Font_7x10, COLOR_TERM_TEXT, BLACK);

            Draw_Terminal_Button(&btn_Opt_Tx, "TRANSMIT", 0);
            Draw_Terminal_Button(&btn_Opt_Rename, "RENAME", 0);
            Draw_Terminal_Button(&btn_Opt_Del, "DELETE", 1);
            Draw_Terminal_Button(&btn_Opt_Back, "< BACK", 0);
            break;
        }

        case PAGE_CONFIRM_DELETE:
        {
            LCD_WriteString("// WARNING", 5, 10, Font_7x10, COLOR_ALERT, BLACK);
            LCD_FillRect(0, 25, 240, 1, COLOR_ALERT);
            LCD_WriteString("CONFIRM DELETE?", 67, 50, Font_7x10, COLOR_TERM_TEXT, BLACK);
            
            char* name = signal_db[selected_slot_idx].name;
            int name_width = strlen(name) * 7;
            int name_x = (240 - name_width) / 2;
            LCD_WriteString(name, name_x, 70, Font_7x10, COLOR_TERM_DIM, BLACK);
            
            Draw_Terminal_Button(&btn_Conf_No, "NO", 1); 
            Draw_Terminal_Button(&btn_Conf_Yes, "YES", 0); 
            break;
        }

        case PAGE_KEYBOARD:
            Draw_Keyboard_Static();
            Update_Input_Display(); 
            break;

        case PAGE_TRANSMITTING:
        {
            LCD_WriteString("// TRANSMITTING", 5, 10, Font_7x10, COLOR_TERM_TEXT, BLACK);
            LCD_FillRect(0, 25, 240, 1, COLOR_TERM_TEXT);
            LCD_WriteString("SENDING:", 92, 80, Font_7x10, COLOR_TERM_DIM, BLACK);
            
            char* name = signal_db[selected_slot_idx].name;
            int name_width = strlen(name) * 7;
            int name_x = (240 - name_width) / 2;
            LCD_WriteString(name, name_x, 100, Font_7x10, COLOR_TERM_TEXT, BLACK);
            
            Draw_Terminal_Button(&btn_Stop, "[ STOP SIGNAL ]", 1);
            break;
        }

        case PAGE_RX_SENSING:
            LCD_WriteString("// SNIFFER_ACTIVE", 5, 10, Font_7x10, COLOR_ALERT, BLACK);
            LCD_FillRect(0, 25, 240, 1, COLOR_ALERT);
            LCD_WriteString("WAITING FOR SIGNAL...", 20, 100, Font_7x10, COLOR_TERM_TEXT, BLACK);
            Draw_Terminal_Button(&btn_Back, "< STOP", 1); 
            break;
    }
    ui_needs_update = 0;
}

void UI_Update_Dynamic_Elements(void) {
    // 1. Keyboard Cursor Blink
    if (currentState == PAGE_KEYBOARD) {
        uint16_t cursor_x = 15 + (strlen(input_buffer) * 7);
        if ((HAL_GetTick() / 500) % 2) LCD_FillRect(cursor_x, 35, 7, 10, COLOR_TERM_TEXT);
        else LCD_FillRect(cursor_x, 35, 7, 10, BLACK);
    }
    
    // 2. Matrix Animation (Only during active operations)
    if (currentState == PAGE_TRANSMITTING || currentState == PAGE_RX_SENSING) {
        if ((HAL_GetTick() % 10) == 0) { 
            char hex[3];
            sprintf(hex, "%02X", rand() % 255);
            uint16_t x = 200 + (rand() % 30);
            uint16_t y = 280 + (rand() % 30);
            
            if (currentState == PAGE_RX_SENSING) {
                 x = 100 + (rand() % 40);
                 y = 140 + (rand() % 40);
            }
            LCD_WriteString(hex, x, y, Font_7x10, COLOR_TERM_DIM, BLACK);
        }
    }
    
    // 3. Mock Sniffer Logic (Simulate Finding Signal)
    if (currentState == PAGE_RX_SENSING) {
        static uint32_t sniff_start = 0;
        if (sniff_start == 0) sniff_start = HAL_GetTick();
        
        if (HAL_GetTick() - sniff_start > 3000) {
            sniff_start = 0;
            int new_slot = Find_Free_Slot();
            if (new_slot != -1) {
                selected_slot_idx = new_slot;
                signal_db[new_slot].is_active = 1; 
                strcpy(input_buffer, ""); 
                kb_mode = 0; kb_shift = 0; // Default to lowercase alpha
                currentState = PAGE_KEYBOARD;
                ui_needs_update = 1;
            }
        }
    }
}

void UI_Handle_Touch(uint16_t x, uint16_t y) {
    int start_idx = list_page * SLOTS_PER_PAGE;
    int total_signals = Get_Total_Active_Signals();

    switch (currentState) {
        case PAGE_BOOT: break;

        case PAGE_MAIN:
            if (Button_IsPressed(btn_Tx, x, y)) {
                Flash_Button(&btn_Tx, "> EXECUTE_PAYLOAD", 0);
                currentState = PAGE_TX_LIST;
                list_page = 0; ui_needs_update = 1;
            }
            if (Button_IsPressed(btn_Rx, x, y)) {
                Flash_Button(&btn_Rx, "> SNIFF_TRAFFIC", 0);
                currentState = PAGE_RX_SENSING;
                ui_needs_update = 1;
            }
            break;

        case PAGE_TX_LIST:
            // Prev Page
            if (list_page > 0 && Button_IsPressed(btn_Prev, x, y)) {
                Flash_Button(&btn_Prev, "< PREV", 0); list_page--; ui_needs_update = 1;
            }
            // Next Page
            else if ((total_signals > (start_idx + SLOTS_PER_PAGE)) && Button_IsPressed(btn_Next, x, y)) {
                Flash_Button(&btn_Next, "NEXT >", 0); list_page++; ui_needs_update = 1;
            }
            // Home
            else if (Button_IsPressed(btn_Back, x, y)) {
                Flash_Button(&btn_Back, "< HOME", 1); currentState = PAGE_MAIN; ui_needs_update = 1;
            }
            // Slot Selection
            else {
                char slot_buf[30];
                for (int i=0; i<3; i++) {
                    int db_idx = start_idx + i;
                    ButtonDef* btn = (i==0) ? &btn_Slot1 : (i==1) ? &btn_Slot2 : &btn_Slot3;
                    if (Button_IsPressed(*btn, x, y)) {
                        if (db_idx < MAX_SLOTS && signal_db[db_idx].is_active) {
                            sprintf(slot_buf, "> %s", signal_db[db_idx].name);
                            Flash_Button(btn, slot_buf, 0);
                            selected_slot_idx = db_idx;
                            currentState = PAGE_OPTIONS; 
                            ui_needs_update = 1;
                        }
                    }
                }
            }
            break;

        case PAGE_OPTIONS:
            if (Button_IsPressed(btn_Opt_Tx, x, y)) {
                Flash_Button(&btn_Opt_Tx, "TRANSMIT", 0);
                currentState = PAGE_TRANSMITTING;
                ui_needs_update = 1;
            }
            else if (Button_IsPressed(btn_Opt_Rename, x, y)) {
                Flash_Button(&btn_Opt_Rename, "RENAME", 0);
                strcpy(input_buffer, signal_db[selected_slot_idx].name);
                kb_mode = 0; kb_shift = 0;
                currentState = PAGE_KEYBOARD;
                ui_needs_update = 1;
            }
            else if (Button_IsPressed(btn_Opt_Del, x, y)) {
                Flash_Button(&btn_Opt_Del, "DELETE", 1);
                currentState = PAGE_CONFIRM_DELETE; 
                ui_needs_update = 1;
            }
            else if (Button_IsPressed(btn_Opt_Back, x, y)) {
                Flash_Button(&btn_Opt_Back, "< BACK", 0);
                currentState = PAGE_TX_LIST;
                ui_needs_update = 1;
            }
            break;

        case PAGE_CONFIRM_DELETE:
            if (Button_IsPressed(btn_Conf_Yes, x, y)) {
                Flash_Button(&btn_Conf_Yes, "YES", 0); 
                Delete_Signal(selected_slot_idx);
                // Fix pagination if page becomes empty
                int new_total = Get_Total_Active_Signals();
                if ((list_page * SLOTS_PER_PAGE) >= new_total && list_page > 0) {
                    list_page--;
                }
                currentState = PAGE_TX_LIST; 
                ui_needs_update = 1;
            }
            else if (Button_IsPressed(btn_Conf_No, x, y)) {
                Flash_Button(&btn_Conf_No, "NO", 1); 
                currentState = PAGE_OPTIONS; 
                ui_needs_update = 1;
            }
            break;

        case PAGE_KEYBOARD:
            if (Button_IsPressed(btn_Kb_Done, x, y)) {
                Flash_Button(&btn_Kb_Done, "OK", 0);
                strcpy(signal_db[selected_slot_idx].name, input_buffer);
                signal_db[selected_slot_idx].is_active = 1;
                Storage_SaveSignals();
                currentState = PAGE_TX_LIST; 
                ui_needs_update = 1; 
            }
            else if (Button_IsPressed(btn_Kb_Del, x, y)) {
                Flash_Button(&btn_Kb_Del, "DEL", 1); 
                int len = strlen(input_buffer);
                if (len > 0) input_buffer[len-1] = '\0';
                Update_Input_Display(); 
            }
            else if (Button_IsPressed(btn_Kb_Space, x, y)) {
                Flash_Button(&btn_Kb_Space, "_", 0);
                int len = strlen(input_buffer);
                if (len < NAME_LEN) {
                    input_buffer[len] = '_';
                    input_buffer[len+1] = '\0';
                }
                Update_Input_Display(); 
            }
            else if (Button_IsPressed(btn_Kb_Mode, x, y)) {
                kb_mode = !kb_mode; 
                const char* new_lbl = (kb_mode==0) ? "123" : "ABC";
                Flash_Button(&btn_Kb_Mode, new_lbl, 0);
                ui_needs_update = 1; 
            }
            else if (Button_IsPressed(btn_Kb_Shift, x, y)) {
                kb_shift = !kb_shift; 
                Flash_Button(&btn_Kb_Shift, "SHF", 0); 
                ui_needs_update = 1; 
            }
            else {
                char k = Check_Keyboard_Touch(x, y);
                if (k != 0) {
                    int len = strlen(input_buffer);
                    if (len < NAME_LEN) {
                        input_buffer[len] = k;
                        input_buffer[len+1] = '\0';
                    }
                    Update_Input_Display(); 
                }
            }
            break;

        case PAGE_TRANSMITTING:
            if (Button_IsPressed(btn_Stop, x, y)) {
                Flash_Button(&btn_Stop, "[ STOP SIGNAL ]", 1);
                currentState = PAGE_TX_LIST;
                ui_needs_update = 1;
            }
            break;

        case PAGE_RX_SENSING:
            if (Button_IsPressed(btn_Back, x, y)) {
                Flash_Button(&btn_Back, "< STOP", 1); 
                currentState = PAGE_MAIN;
                ui_needs_update = 1;
            }
            break;
    }
}
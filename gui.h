// gui.h

// game screen, graphic interface, event management

#pragma once

#include "common.h"
#include "log.h"
#include "bbot.h"
#include "board.h"
#include "game.h"
#include "external\SDL2-2.26.2\include\SDL.h"
#include "external\SDL2_image-2.6.2\include\SDL_image.h"
#include "external\SDL2_ttf-2.20.1\include\SDL_ttf.h"

namespace Bbot2 {

class GUI {

	//// INITIAL SCREEN WIDTH/HEIGHT ////
	int SCREEN_WIDTH = 800; 
	int SCREEN_HEIGHT = 1050;

	//// LAYOUT CONSTANTS ////
	const int MARGIN = 50;
	const int MIN_GAME_SIZE = 400;
	const int INFO_BOX_HEIGHT = 200;

	//// COLOURS ////
	SDL_Color COLOUR_BG =			{ 0x20, 0x20, 0x20, 0xFF }; // background
	SDL_Color COLOUR_SQUARES[2] = {
									{ 0xBF, 0xD4, 0xDB, 0xFF },  // white squares
									{ 0x74, 0xAD, 0xC2, 0xFF }	// black squares
	};
	SDL_Color COLOUR_SELECTED =		{ 0xFF, 0xFF, 0x88, 0x66 }; // if piece is selected
	SDL_Color COLOUR_FORCED =		{ 0xFF, 0x00, 0x00, 0x66 }; // if piece is forced
	SDL_Color COLOUR_THREAT =		{ 0x00, 0x00, 0x00, 0x66 }; // if piece is threatened but not forced
	SDL_Color COLOUR_HOVER =		{ 0xFF, 0xFF, 0xFF, 0x66 }; // if mouse over legal square while piece is grabbed
	SDL_Color COLOUR_LAST_MOVE =	{ 0xFF, 0xFF, 0x88, 0x66 }; // highlight last move
	SDL_Color COLOUR_COORD =		{ 0xAA, 0xAA, 0xAA, 0xFF }; // coordinate text colour
	SDL_Color COLOUR_TEXT =			{ 0xFF, 0xFF, 0xFF, 0xFF }; // game info text colour


	//// RESOURCES ////
	const std::string RESOURCES_FOLDER = "resources/";

	// PNG format expected
	const std::string MOVE_INDICATOR_FILE = "move_indicator.png";
	const std::string WH_MASK_FILE = "wh_mask.png"; // watering hole mask
	const std::string HOVER_MASK_FILE = "hover_mask.png";

	const std::string PIECE_SHEET_FILE = "piece_sheet.png";
	// arrangment must be
	// | MW LW EW |
	// | MB LB EB |
	// and resolution (PIECE_TEXTURE_WIDTH * 3) x (PIECE_TEXTURE_HEIGHT * 2)
	// for best results

	const int PIECE_TEXTURE_WIDTH = 240; // cropped size of individual texture
	const int PIECE_TEXTURE_HEIGHT = 240;

	//// FONT ////
	const std::string FONT_FILE = "consola.ttf";
	static const int FONT_SIZE = 16;

	////



	// layout variables
	int GAME_X, GAME_Y, GAME_WIDTH, GAME_HEIGHT, SQUARE_WIDTH, SQUARE_HEIGHT;
	bool showInfo = true;

	SDL_Renderer* renderer; // renderer
	SDL_Window* window; // window
	SDL_Event event; // address used by event handler
	bool requestQuit = false; // set true to exit

	// texture objects
	std::vector<SDL_Texture*> textureList; // populated by add_texture, used to de-allocate on close
	SDL_Texture* pieceTexture;
	SDL_Texture* moveTexture;
	SDL_Texture* whTexture[2]; // black, white
	SDL_Texture* hoverTexture;

	TTF_Font* font; // font

	// text objects, populated by init_text
	class Text;
	std::vector<Text> rowCoords;
	std::vector<Text> colCoords;
	std::vector<Text> infoText;
	// infoText
	//	[0] - "SEARCHING..." if search is active
	//	[1] - eval from last computer to play
	//	[2] - PV from last computer to play
	//	[3] - played line

	// cursors
	SDL_SystemCursor SYSTEM_CURSOR[3] = { SDL_SYSTEM_CURSOR_ARROW, SDL_SYSTEM_CURSOR_WAITARROW, SDL_SYSTEM_CURSOR_HAND };
	SDL_Cursor* cursorList[3]; // 0 = arrow, 1 = wait, 2 = hand

	Game* game; // interface for board and comp players
	Board* board; // from game - points to attached board
	std::vector<Piece*>* pieces; // from board - consecutive piece array

	Piece* displayBoard[NUM_SQUARES]; // pointerBoard for display
	int moveIndex = 0; // index of game->playedLine to use for display, allows viewing past positions

	bool flipBoard = false; // false - white's view, true - black's view

	int mouseScalar = -1; // scalar of mouse position on board, -1 is off board
	Piece* selected = nullptr; // points to selected piece (highlighted, showing legal moves)
	Piece* grabbed = nullptr; // points to grabbed piece (lifted off the board)
	

public:
	GUI(Game* game_);

	void init();
	void open_game(Game* game_);

	void update();

	void draw();

	bool loop_end();
	void close();

private:
	void init_SDL();

	void load_resources();
	SDL_Texture* load_PNG(std::string filename);
	SDL_Texture* load_PNG_colour(std::string maskFile, SDL_Color colour);
	SDL_Texture* add_texture(SDL_Texture* t);

	void handle_events();
	void on_mouse_move_event();
	void on_mouse_down_event();
	void on_mouse_up_event();
	void on_key_down_event(SDL_Keycode keycode);
	void window_resized(int width, int height);
	void quit_event();

	void on_move_played();
	void set_move_index(int index);
	bool is_display_current();

	void init_text();
	void update_text();

	void draw_squares();
	void draw_coords();
	void draw_info();
	void draw_pieces();

	int scalar_to_x(int k);
	int scalar_to_y(int k);
	int x_y_to_scalar(int x, int y);
	int scalar_to_colour(int k);
	
	void free_resources();

	// nested class for text items
	class Text {
	public:
		std::string text;
		int x; // coordinates x, y
		int y;
		int dx; // display x
		int width; // display width
		int textWidth; // text width/height
		int textHeight;
		int align; // TTF_WRAPPED_ALIGN_LEFT, TTF_WRAPPED_ALIGN_CENTER, TTF_WRAPPED_ALIGN_RIGHT
		SDL_Color colour;
		SDL_Texture* texture;
		size_t textureIndex = SIZE_MAX; // index within textureList, SIZE_MAX if not rendered
		GUI* graphics;

		Text(std::string text_, int x_, int y_, int width_, int align_, SDL_Color colour_, GUI* graphics_);

		void render();

		void write(std::string text_);
		void set_width(int width_);

		void draw();
	};

	void __DEBUG(bool condition, std::string message);
};

} // end namespace Bbot2
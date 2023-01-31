// gui.cpp

#include "gui.h"

using std::string;
using std::format;


namespace Bbot2 {

// Game //

// constructor
GUI::GUI(Game* game_)
	: game(game_) {

	board = game->board;
	pieces = board->pieces;
}

////

// initialize
void GUI::init() {

	// initialize SDL
	init_SDL();

	window_resized(screenWidth, screenHeight); // sets layout values

	// load textures, fonts, cursors
	load_resources();

	// initialize text objects
	init_text();

	// copy display board
	std::copy(board->pointerBoard, &board->pointerBoard[NUM_SQUARES], displayBoard);

	// display move index
	moveIndex = 0;

	// initial board orientation
	flipBoard = PERSPECTIVE == FIXED_BLACK;
}

// reset when passed a new game
void GUI::open_game(Game* game_) {
	game = game_;
	board = game->board;
	pieces = board->pieces;

	// copy display board
	std::copy(board->pointerBoard, &board->pointerBoard[NUM_SQUARES], displayBoard);

	// display move index
	moveIndex = 0;
}

////

// main update
void GUI::update() {
	// events
	handle_events();

	// text
	update_text();

	if (game->movePlayed)
		on_move_played();
}

////

// main draw
void GUI::draw() {
	// clear screen
	SDL_SetRenderDrawColor(renderer, COLOUR_BG.r, COLOUR_BG.g, COLOUR_BG.b, COLOUR_BG.a);
	SDL_RenderClear(renderer);

	// render texture to screen
	//SDL_RenderCopy(renderer, pieceTextures[0], NULL, NULL);
	draw_squares();

	draw_coords();

	draw_pieces();

	draw_info();

	// update screen
	SDL_RenderPresent(renderer);
}

////

// returns true when window should be exited
bool GUI::loop_end() {
	return requestQuit;
}

// free all resources, quit SDL
void GUI::close() {
	// free resources
	free_resources();

	// destroy window
	SDL_DestroyWindow(window);

	// destroy renderer
	SDL_DestroyRenderer(renderer);

	// quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

////////////////////////////////

// initialize SDL and necessary objects
void GUI::init_SDL() {
	// initialize 
	SDL_Init(SDL_INIT_VIDEO);

	// set texture filtering to linear
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	// create window
	window = SDL_CreateWindow("BBOT 2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	// set renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE);

	// blends alpha
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// initialize SDL accessories
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
}

// load all necessary graphics/resources
void GUI::load_resources() {

	// piece texture sheet
	pieceTexture = load_PNG(PIECE_SHEET_FILE);

	// move indicator texture
	moveTexture = load_PNG(MOVE_INDICATOR_FILE);

	// watering hole textures, using opposite square colours
	for (int i = 0; i < 2; i ++) {
		whTexture[i] = load_PNG_colour(WH_MASK_FILE, COLOUR_SQUARES[1 - i]);
	}

	// hover texture, using hover colour
	hoverTexture = load_PNG_colour(HOVER_MASK_FILE, COLOUR_HOVER);


	// font
	font = TTF_OpenFont((RESOURCES_FOLDER + FONT_FILE).c_str(), FONT_SIZE);


	// system cursors
	for (int i = 0; i < 3; i ++) {
		cursorList[i] = SDL_CreateSystemCursor(SYSTEM_CURSOR[i]);
	}
}

// create texture from PNG
SDL_Texture* GUI::load_PNG(string filename) {
	filename = RESOURCES_FOLDER + filename; // full file name

	// create surface to load file
	SDL_Surface* surface = IMG_Load(filename.c_str());
	__DEBUG(surface == NULL, "Failed to load \"" + filename + "\"");

	// create texture from file surface
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

	// free surface
	SDL_FreeSurface(surface);

	return add_texture(texture); // add to textureList
}

// create texture using an alpha mask and a colour
// - mask should be a PNG containing only black with desired alpha values
// - any black will be blended away using "add" blendmode, leaving only the desired colour, now with the mask's alpha
// - the texture's alpha will be scaled by the colour's original alpha
SDL_Texture* GUI::load_PNG_colour(string maskFile, SDL_Color colour) {
	SDL_Surface* maskSurface, * colourSurface;

	// load mask
	maskFile = RESOURCES_FOLDER + maskFile;
	maskSurface = IMG_Load(maskFile.c_str());

	__DEBUG(maskSurface == NULL, "Failed to load \"" + maskFile + "\"");

	// dimensions
	SDL_Rect rect = { 0, 0, maskSurface->w, maskSurface->h };

	// create plain rectangle with colour wanted
	colourSurface = SDL_CreateRGBSurfaceWithFormat(0, maskSurface->w, maskSurface->h, maskSurface->format->BitsPerPixel, SDL_PIXELFORMAT_RGBA32);
	SDL_FillRect(colourSurface, &rect, SDL_MapRGB(colourSurface->format, colour.r, colour.g, colour.b));

	// blend colour with mask using "add" blendmode
	SDL_SetSurfaceBlendMode(colourSurface, SDL_BLENDMODE_ADD);
	SDL_BlitScaled(colourSurface, NULL, maskSurface, NULL);

	// create texture
	SDL_Texture* texture = add_texture(SDL_CreateTextureFromSurface(renderer, maskSurface));

	// scale texture alpha by colour alpha value when rendering
	SDL_SetTextureAlphaMod(texture, colour.a);

	// free surfaces (mask was written over, so it needs to load again)
	SDL_FreeSurface(colourSurface);
	SDL_FreeSurface(maskSurface);

	return texture;
}

// remember texture
// used to later de-allocate all textures at close
SDL_Texture* GUI::add_texture(SDL_Texture* t) {
	textureList.push_back(t);
	return t;
}

////

// main event handler
void GUI::handle_events() {

	// event queue
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: // quit
				quit_event();
				return;

			case SDL_MOUSEMOTION: 
				on_mouse_move_event(); // mouse move
				break; 
			case SDL_MOUSEBUTTONDOWN: // mouse button down
				on_mouse_down_event();
				break; 
			case SDL_MOUSEBUTTONUP: // mouse button up
				on_mouse_up_event();
				break; 

			case SDL_KEYDOWN: // key down
				on_key_down_event(event.key.keysym.sym);
				break;

			case SDL_WINDOWEVENT: // window resized
				// supposed to be continuous according to documentation, seems to be broken in current SDL2
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					window_resized(event.window.data1, event.window.data2);
				break;
		}
	}
}

// on mouse move
// sets mouseScalar and current cursor
void GUI::on_mouse_move_event() {

	// if not user, set wait cursor and exit
	if (!game->user_to_play()) { 
		mouseScalar = -1;
		SDL_SetCursor(cursorList[1]); // set wait cursor
		return;
	}

	if (!is_display_current()) {
		SDL_SetCursor(cursorList[0]);
		mouseScalar = -1;
		return;
	}

	
	// get mouse pos
	int x, y;
	SDL_GetMouseState(&x, &y);

	// scalar representation, -1 if off board
	mouseScalar = x_y_to_scalar(x, y);
	
	Piece* p = mouseScalar == -1 ? nullptr : board->pointerBoard[mouseScalar]; // get piece under mouse
	SDL_Cursor* cursor = cursorList[0]; // set default arrow cursor

	// if mouse over piece on board of current side
	if (p != nullptr)
		if (p->side == SIDES[board->sideToMove])
			cursor = cursorList[2]; // set hand cursor

	// or if piece is grabbed
	if (grabbed != nullptr)
		cursor = cursorList[2]; // set hand cursor

	SDL_SetCursor(cursor);
}

// on mouse down
// grabs and selects piece, if mouse over piece 
void GUI::on_mouse_down_event() {

	// if comp playing or mouse is off board, exit
	if (!game->user_to_play() || mouseScalar == -1)
		return;

	// get piece under mouse
	Piece* p = board->pointerBoard[mouseScalar];

	// if mouse over piece of current side
	if (p != nullptr) {
		if (p->side == SIDES[board->sideToMove]) {
			grabbed = p;
			selected = p;
		}
	}
}

// on mouse up
// attempt to play move with grabbed/selected piece
void GUI::on_mouse_up_event() {
	// if comp playing, exit
	if (!game->user_to_play())
		return;

	// if piece grabbed or selected and mouse is released, attempt to play move
	if (mouseScalar != -1) {
		if (grabbed != nullptr) {
			game->play_move(Move(grabbed->scalar, mouseScalar));
			on_move_played();
		} else if (selected != nullptr && mouseScalar != -1) {
			game->play_move(Move(selected->scalar, mouseScalar));
			selected = nullptr; // only clear selected when not grabbed
			on_move_played();
		}
	}

	// clear grabbed
	grabbed = nullptr;
}

// key down event
void GUI::on_key_down_event(SDL_Keycode keycode) {
	switch (keycode) {
		case SDLK_LEFT:
			set_move_index(std::max(moveIndex - 1, 0));
			break;

		case SDLK_RIGHT:
			set_move_index(std::min(moveIndex + 1, game->playedLine.length));
			break;
	}
}


// window resized
// called when the screen is initialized or resized
// updates layout values
void GUI::window_resized(int width, int height) {
	// update screen width/height
	screenWidth = width;
	screenHeight = height;

	// if screen is too short, don't show info box
	showInfo = MARGIN + MIN_GAME_SIZE + MARGIN + INFO_BOX_HEIGHT + MARGIN <= screenHeight;

	// game size, with or without info box
	if (showInfo) {
		GAME_WIDTH = std::min(screenWidth, screenHeight - INFO_BOX_HEIGHT) - 2 * MARGIN;
	} else {
		GAME_WIDTH = std::min(screenWidth, screenHeight) - 2 * MARGIN;
	}

	// game width/height can't go below minimum
	GAME_WIDTH = std::max(GAME_WIDTH, MIN_GAME_SIZE);
	GAME_HEIGHT = GAME_WIDTH;

	// game x/y
	GAME_X = std::max(screenWidth / 2 - GAME_WIDTH / 2, MARGIN);
	GAME_Y = MARGIN;

	// square width/height
	SQUARE_WIDTH = GAME_WIDTH / BOARD_SIZE;
	SQUARE_HEIGHT = GAME_HEIGHT / BOARD_SIZE;

	// round game width/height to the nearest BOARD_SIZE for easier drawing/calculation
	GAME_WIDTH = SQUARE_WIDTH * BOARD_SIZE;
	GAME_HEIGHT = SQUARE_HEIGHT * BOARD_SIZE;
}

void GUI::quit_event() {
	requestQuit = true;

	for (int i = 0; i < 2; i ++)
		if (game->players[i] != nullptr)
			game->players[i]->search_abort();
}

////

// call after game->play_move()
void GUI::on_move_played() {

	// clear grabbed/selected
	grabbed = nullptr;
	selected = nullptr;

	// update display board
	set_move_index(game->playedLine.length);

	// flip board if needed
	if (PERSPECTIVE == AUTO && game->user_to_play())
		flipBoard = board->sideToMove;
}

// set played line index, determining what position of the game to show
void GUI::set_move_index(int i) {

	Move m;
	while (moveIndex < i) {
		m = game->playedLine.get_move(moveIndex);
		displayBoard[m.get_to()] = displayBoard[m.get_from()];
		displayBoard[m.get_from()] = nullptr;
		moveIndex ++;
	}

	while (moveIndex > i) {
		m = game->playedLine.get_move(moveIndex - 1);
		displayBoard[m.get_from()] = displayBoard[m.get_to()];
		displayBoard[m.get_to()] = nullptr;
		moveIndex --;
	}
}

// if the board being displayed is current
bool GUI::is_display_current() {
	return moveIndex == game->playedLine.length;
}

////

// create all text items
void GUI::init_text() {

	// coordinate labels
	rowCoords.clear();
	colCoords.clear();
	for (int i = 0; i < BOARD_SIZE; i ++) {
		// 1 - 10
		rowCoords.push_back(Text(std::to_string(i + 1), 0, 0, 0, TTF_WRAPPED_ALIGN_CENTER, COLOUR_COORD, this));
		// a - j
		colCoords.push_back(Text(string(1, FILE_CHARS[i]), 0, 0, 0, TTF_WRAPPED_ALIGN_CENTER, COLOUR_COORD, this));
	}

	// info text
	//	[0] - "SEARCHING..." if search is active
	//	[1] - eval from last computer to play
	//	[2] - PV from last computer to play
	//	[3] - played line

	infoText.clear();
	for (int i = 0; i < 4; i ++)
		infoText.push_back(Text("", 0, 0, 0, TTF_WRAPPED_ALIGN_LEFT, COLOUR_TEXT, this));

	infoText[3].colour = COLOUR_COORD;
}

// write contents of text
void GUI::update_text() {
	if (game->game_over()) {

		// text for end of game
		infoText[0].write("");

		switch (game->outcome) {
			case WIN_WHITE: infoText[1].write("GAME OVER, WHITE WINS"); break;
			case WIN_BLACK: infoText[1].write("GAME OVER, BLACK WINS"); break;
			case DRAW_REPETITION: infoText[1].write("GAME OVER, DRAW BY REPITITION"); break;
			case DRAW_MOVE_LIMIT: infoText[1].write("GAME OVER, REACHED MOVE LIMIT"); break;
		}

		infoText[2].write("");
	} else {

		// get info about last search from game
		infoText[0].write(game->searching ? "SEARCHING..." : "");
		infoText[1].write(format("EVAL: {}, DEPTH: {} ({:.2f}s)", game->searchEval, game->searchDepth, game->searchDuration));
		infoText[2].write(game->searchPV);
		infoText[3].write(game->playedLine.to_string(board->startPointerBoard));
	}
}



// draw white/black squares that compose the game board
void GUI::draw_squares() {
	
	// all white squares as one large, background rect
	SDL_Rect rect = { GAME_X, GAME_Y, GAME_WIDTH, GAME_HEIGHT };
	SDL_SetRenderDrawColor(renderer, COLOUR_SQUARES[WHITE].r, COLOUR_SQUARES[WHITE].g, COLOUR_SQUARES[WHITE].b, COLOUR_SQUARES[WHITE].a);
	SDL_RenderFillRect(renderer, &rect);

	// iterate through to color black squares as individual rects
	SDL_SetRenderDrawColor(renderer, COLOUR_SQUARES[BLACK].r, COLOUR_SQUARES[BLACK].g, COLOUR_SQUARES[BLACK].b, COLOUR_SQUARES[BLACK].a);
	rect.w = SQUARE_WIDTH;
	rect.h = SQUARE_HEIGHT;

	for (int i = 0; i < BOARD_SIZE; i ++) {
		for (int j = 0; j < BOARD_SIZE; j ++) {
			if (scalar_to_colour((BOARD_SIZE - 1 - i) * BOARD_SIZE + j)) { // only colour black squares
				rect.x = GAME_X + j * SQUARE_WIDTH;
				rect.y = GAME_Y + i * SQUARE_HEIGHT;

				SDL_RenderFillRect(renderer, &rect);
			}
		}
	}


	// draw watering holes
	for (int k : board->whScalars) {
		rect.x = scalar_to_x(k);
		rect.y = scalar_to_y(k);;

		SDL_RenderCopy(renderer, whTexture[scalar_to_colour(k)], NULL, &rect);
	}

	if (!is_display_current())
		return;

	// highlight last move
	if (moveIndex > 0) {
		Move move = game->playedLine.get_move(moveIndex - 1);

		SDL_SetRenderDrawColor(renderer, COLOUR_LAST_MOVE.r, COLOUR_LAST_MOVE.g, COLOUR_LAST_MOVE.b, COLOUR_LAST_MOVE.a);

		// from
		rect.x = scalar_to_x(move.get_from());
		rect.y = scalar_to_y(move.get_from());
		SDL_RenderFillRect(renderer, &rect);

		// to
		rect.x = scalar_to_x(move.get_to());
		rect.y = scalar_to_y(move.get_to());
		SDL_RenderFillRect(renderer, &rect);
	}

	// draw forced squares under forced pieces
	SDL_SetRenderDrawColor(renderer, COLOUR_FORCED.r, COLOUR_FORCED.g, COLOUR_FORCED.b, COLOUR_FORCED.a);
	
	for (Side side : SIDES) {
		for (Piece* p : pieces[side]) {

			// if piece is forced
			if (p->isForced) {
				rect.x = scalar_to_x(p->scalar);
				rect.y = scalar_to_y(p->scalar);

				SDL_RenderFillRect(renderer, &rect);
			}
		}
	}

	// draw threatened squares under threatened but not forced pieces
	SDL_SetRenderDrawColor(renderer, COLOUR_THREAT.r, COLOUR_THREAT.g, COLOUR_THREAT.b, COLOUR_THREAT.a);

	for (Side side : SIDES) {
		for (Piece* p : pieces[side]) {

			// if piece threatened but not forced
			if (p->isThreatened && !p->isForced) {
				rect.x = scalar_to_x(p->scalar);
				rect.y = scalar_to_y(p->scalar);

				SDL_RenderFillRect(renderer, &rect);
			}
		}
	}
}

// draw pieces, hover, selected square, and move indicators
void GUI::draw_pieces() {
	SDL_Rect srcRect, destRect;

	// if a piece is selected
	if (selected != nullptr && is_display_current()) {
		destRect = { 0, 0, SQUARE_WIDTH, SQUARE_HEIGHT };

		// draw selected square under
		SDL_SetRenderDrawColor(renderer, COLOUR_SELECTED.r, COLOUR_SELECTED.g, COLOUR_SELECTED.b, COLOUR_SELECTED.a);
		destRect.x = scalar_to_x(selected->scalar);
		destRect.y = scalar_to_y(selected->scalar);

		SDL_RenderFillRect(renderer, &destRect);

		// draw move indicator for every legal move
		for (int i = 0; i < NUM_SQUARES; i ++) {
			if (selected->moveBoard[i]) {
				destRect.x = scalar_to_x(i);
				destRect.y = scalar_to_y(i);
				SDL_RenderCopy(renderer, moveTexture, NULL, &destRect);
			}
		}
	}

	srcRect = { 0, 0, PIECE_TEXTURE_WIDTH, PIECE_TEXTURE_HEIGHT };
	destRect = { 0, 0, SQUARE_WIDTH, SQUARE_HEIGHT };


	// draw pieces
	Piece* p;
	for (int i = 0; i < NUM_SQUARES; i ++) {
		p = displayBoard[i];

		if (p == nullptr)
			continue;

		// get piece position in texture sheet
		srcRect.x = PIECE_TEXTURE_WIDTH * p->type;
		srcRect.y = PIECE_TEXTURE_HEIGHT * p->side;

		if (p == grabbed) {
			// if grabbed, place under mouse
			SDL_GetMouseState(&destRect.x, &destRect.y);
			destRect.x -= SQUARE_WIDTH / 2;
			destRect.y -= SQUARE_HEIGHT / 2;
		} else {
			// otherwise use piece scalar position
			destRect.x = scalar_to_x(i);
			destRect.y = scalar_to_y(i);
		}

		// draw piece
		SDL_RenderCopy(renderer, pieceTexture, &srcRect, &destRect);
	}

	// draw hover if piece is grabbed
	if (grabbed != nullptr && mouseScalar != -1) {
		destRect.x = scalar_to_x(mouseScalar);
		destRect.y = scalar_to_y(mouseScalar);

		SDL_RenderCopy(renderer, hoverTexture, NULL, &destRect);
	}
}

// update position of and draw coordinate labels
void GUI::draw_coords() {

	for (int i = 0; i < BOARD_SIZE; i ++) {
		// rows
		rowCoords[i].x = GAME_X - FONT_SIZE;
		rowCoords[i].y = scalar_to_y(i * BOARD_SIZE) + SQUARE_HEIGHT / 2 - FONT_SIZE / 2;
		rowCoords[i].draw();

		// cols
		colCoords[i].x = scalar_to_x(i) + SQUARE_WIDTH / 2;
		colCoords[i].y = GAME_Y + GAME_HEIGHT + FONT_SIZE;
		colCoords[i].draw();
	}
}

// draw info box under game board
void GUI::draw_info() {

	// iterate through infoText, update pos and width, then draw and move to next line
	int y = GAME_Y + GAME_HEIGHT + MARGIN;
	for (int i = 0; i < infoText.size(); i ++) {
		infoText[i].x = MARGIN;
		infoText[i].y = y;
		infoText[i].set_width(screenWidth - MARGIN * 2);
		infoText[i].draw();
		y += infoText[i].textHeight + FONT_SIZE / 2;
	}
}

////

// utilities for draw and update functions
// converts scalar on board to drawn x on screen
int GUI::scalar_to_x(int k) {
	return GAME_X + SQUARE_WIDTH * (flipBoard ? BOARD_SIZE - 1 - (k % BOARD_SIZE) : k % BOARD_SIZE);
}

// converts scalar on board to drawn y on screen
int GUI::scalar_to_y(int k) {
	return GAME_Y + SQUARE_HEIGHT * (flipBoard ? k / BOARD_SIZE : BOARD_SIZE - 1 - k / BOARD_SIZE);
}

// converts x y on screen to scalar on board
// returns -1 if off board
int GUI::x_y_to_scalar(int x, int y) {
	if (x < GAME_X || y < GAME_Y || x >= GAME_X + GAME_WIDTH || y >= GAME_Y + GAME_HEIGHT)
		return -1;

	x -= GAME_X;
	y -= GAME_Y;
	x = flipBoard ? GAME_WIDTH - x : x;
	y = flipBoard ? y : GAME_HEIGHT - y;
	return (BOARD_SIZE * y / GAME_HEIGHT) * BOARD_SIZE + BOARD_SIZE * x / GAME_WIDTH;
}

// converts scalar to colour (0 = white, 1 = black) of square on board
int GUI::scalar_to_colour(int k) {
	return ((k / BOARD_SIZE) + (k % BOARD_SIZE)) % 2 == 0;
}

////

// de-allocate resources on close
void GUI::free_resources() {
	// free textures
	for (SDL_Texture* t : textureList) {
		SDL_DestroyTexture(t);
	}

	// free cursors
	for (int i = 0; i < 3; i ++) {
		SDL_FreeCursor(cursorList[i]);
	}
}

////////////////////////////////

// Text nested class
// width: if 0, will shift left as needed to align to x. if >0, text will align and wrap within width
// game: point to containing GUI class
GUI::Text::Text(string text_, int x_, int y_, int width_, int align_, SDL_Color colour_, GUI* graphics_)
	: text(text_), x(x_), y(y_), width(width_), align(align_), colour(colour_), graphics(graphics_) {

	// set values
	textWidth = 0;
	textHeight = GUI::FONT_SIZE;
	
	// render
	render();
}

// create texture
void GUI::Text::render() {
	// don't render if empty string. RenderText would return NULL regardless
	if (text == "")
		return;

	// set alignment
	TTF_SetFontWrappedAlign(graphics->font, align);

	// create surface
	SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(graphics->font, text.c_str(), colour, width);

	// remember text width/height
	// not easily accessed from the texture object
	textWidth = surface->w;
	textHeight = surface->h;

	// if texture is indexed, free old texture
	if (textureIndex != -1)
		SDL_DestroyTexture(texture);

	// create texture
	texture = SDL_CreateTextureFromSurface(graphics->renderer, surface);

	// if texture is not indexed, do so now
	if (textureIndex == SIZE_MAX) {
		textureIndex = graphics->textureList.size();
		graphics->add_texture(texture);
	} else {
		// if already indexed, replace old texture
		graphics->textureList[textureIndex] = texture;
	}

	// free surface
	SDL_FreeSurface(surface);
}

// create new texture if new text is given
void GUI::Text::write(string text_) {
	if (text != text_) {
		text = text_;
		render();
	}
}

// create new texture if new width is given
void GUI::Text::set_width(int width_) {
	if (width != width_) {
		width = width_;
		render();
	}
}

// draw text
void GUI::Text::draw() {
	// don't draw if empty string
	if (text == "")
		return;

	// set alignment
	TTF_SetFontWrappedAlign(graphics->font, align);

	SDL_Rect destRect = { x, y, textWidth, textHeight };

	// shift left to align if display width is 0
	if (width == 0) {
		if (align == TTF_WRAPPED_ALIGN_CENTER)
			destRect.x = x - textWidth / 2;

		if (align == TTF_WRAPPED_ALIGN_RIGHT)
			destRect.x = x - textWidth;
	}
	
	SDL_RenderCopy(graphics->renderer, texture, NULL, &destRect);
}

////

void GUI::__DEBUG(bool condition, string message) {
	if (board)
		board->__DEBUG(condition, message);
}

} // end namespace Bbot2
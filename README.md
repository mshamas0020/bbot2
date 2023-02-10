----------------
Bbot2 v0.1
----------------

See SETTINGS.ini for options.
----------------

Barca is a chess variant board game created by Andrew Caldwell. An adapted version of
the rules are included below.

	BARCA – The Watering Hole Game
	For 2 players. Ages 8 to adult.

	OBJECT OF THE GAME
	Get any 3 of your pieces on the watering holes at the same time.
 
	SET UP
	Set the game board between both players. Place the queens in the center of
	each player’s back row. Place the rooks in front of the queens. And place the
	bishops to the sides of the rooks.
	(NOTE: The original board game uses mice, lions, and elephants. In this program,
	the pieces and terminology have been replaced by their chess equivalents.)
 
	RULES OF PLAY
	White has the first move.
	(NOTE: The original board game has players decide first move with Rock Paper
	Scissors.)

	Rooks move horizontally or vertically. Bishops only move diagonally. Queens
	move horizontally, vertically or diagonally. Pieces can move any number of
	vacant squares in a single direction. Pieces cannot jump over other pieces.

	A players’ pieces can be adjacent to each other. However, for opposing sides,
	rooks fear bishops. Bishops fear queens. And queens fear rooks.

	A piece cannot be moved next to a piece it fears, except for trapped
	pieces.

	A piece is scared when it is next to a piece it fears.

	A scared piece must escape to a safe location before other pieces can be
	moved. When many are scared, choose one to move.

	A scared piece with no escape is trapped and may optionally move to another
	scared position.

	Watering holes are near the center of the game board. The player who has
	pieces on 3 watering holes at the same time wins!

	A player can still win if pieces are scared or trapped. Pieces are never
	removed.
	
Original rules: http://playbarca.com/?page_id=40
----------------

Bbot2 was designed as a chess variant engine to play the game as optimally as
possible. Many traditional chess programming techniques were borrowed, with exact
implementations changed to fit the variant rule-set. The search currently uses a
negamax alphabeta tree with a PV-estimated aspiration window and transposition
tables.

Credit to the creators of chessprogramming.org and Bruce Moreland for the generous
amount of educational material on the topic.
Third-party software includes SDL2 and SimpleINI, found in /external.

This program contains a GUI and can run portably with only bbot2.exe, SETTINGS.ini, .dlls,
and resources/.
All binaries are for Windows x64.

Please email matt(@)shamas(.)ca with any questions or bugs.

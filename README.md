================
Bbot2 v1.0
================

See SETTINGS.ini for options.
================

Barca is a chess variant board game created by Andrew Caldwell. The rules are included
below.
	*
	BARCA – The Watering Hole Game
	For 2 players. Ages 8 to adult.

	OBJECT OF THE GAME
	Get any 3 of your animals on the watering holes at the same time.
 
	SET UP
	Set the game board between both players. Place the elephants in the center of
	each player’s back row. Place the mice in front of the elephants. And place the
	lions to the sides of the mice.
 
	RULES OF PLAY
	The winner of Rock Paper Scissors (Roshambo) starts the game. Players take
	turns moving one of their animals.
	(NOTE: This program has white start.)

	Mice move horizontally or vertically. Lions only move diagonally. Elephants
	move horizontally, vertically or diagonally. Animals can move any number of
	vacant squares in a single direction. Animals cannot jump over other animals.

	A players’ animals can be adjacent to each other. However, for opposing sides,
	mice fear lions. Lions fear elephants. And elephants fear mice.

	An animal cannot be moved next to the animal it fears, except for trapped
	animals.

	An animal is scared when it is next to the animal it fears.

	A scared animal must escape to a safe location before other animals can be
	moved. When many are scared, choose one to move.

	A scared animal with no escape is trapped and may optionally move to another
	scared position.

	Watering holes are near the center of the game board. The player who has
	animals on 3 watering holes at the same time wins!

	A player can still win if animals are scared or trapped. Animals are never
	removed.
 
	Have Fun!
	*
See http://playbarca.com/?page_id=40
================

Bbot2 was designed as a chess variant engine to play the game as optimally as
possible. Many traditional chess programming techniques were borrowed, with exact
implementations changed to fit the variant rule-set. The search currently uses a
negamax alphabeta tree with a PV-estimated aspiration window and transposition
tables.

Credit to the creators of chessprogramming.org and Bruce Moreland for the exhaustive
amount of educational material on the topic.
Third-party software includes SDL2 and SimpleINI, contained in /external.

x64 architecture is required.
Please email matt(@)shamas(.)ca with any questions or bug reports.

-- Matt
 to play the game:
-launch client/win32_client.exe
-in the launcher screen, you can decide to select your roles and start the editor, or to start the "vanilla" server
-after having started the editor/server, join it by clicking on the central button
In the editor mode you'll see there are a lot of widgets around the screen, depending on the role you've selected at startup.
The editor mode allows you to modify EVERYTHING in the game, plants, rocks, animals, ground, world generators, sounds.
-to hide a widget, just click on it's name.
-the "taxonomy tree" and "editor tabs" widgets are probably the two most important ones: the first one contains the taxonomy tree of the entire game, allowing the game to detect what is what. The editor tab widget allows you to alter the properties of a specific taxonomy.
-Also, if you are in editor mode and you hover over something keeping pressed the alt button, you'll see you will get a shortcut to edit that taxonomy.
-at the moment you won't be able to "alter" the taxonomy tree if you don't have saved all the modified taxonomy, so if you want to add another kind of tree make sure you've saved everything.
You spot modified taxonomy in red in the taxonomy tree widget.
You can also instantiate a taxonomy by pressing the "place" button in the widget taxonomy and releasing it where you want to place the entity.
If the taxonomy you've selected is not a "leaf" of the tree, but a branch, the final taxonomy will be randomly selected. For example if you instantiate a wolf, then a wolf will be placed, but it you place an "animal", a random animal will spawn.
You can also change the world seed in the "misc" widget, and recreate the world from scratch clicking on the "recreate world" button in the action widget.







Procedural Rock generation has been implemented following the awesome paper by Anders Söderlund. (http://www.diva-portal.org/smash/get/diva2:817559/FULLTEXT01.pdf)
Editor Rock Param List:
-color, average coloration of the rock
-startingColorDelta, how much the starting color can vary from rock to rock
-perVertexColorDelta, how much the color can vary when new vertex are created from the previous vertexes
-iterations, how many times new vertexes are created from old vertexes, following the starting specified model
-minDisplacementY, new vertexes min displacement along the Y axis
-maxDisplacementY, new vertexes max displacement along the Y axis
-minDisplacementZ, new vertexes min displacement along the Z axis
-maxDisplacementZ, new vertexes max displacement along the Z axis
-smoothness, if 0, new vertexes will follow at 100% the color of one of the "generating" vertexes, if 1 it will perfectly blend between the two
-smoothnessDelta, how much the smoothness can vary from vertex to vertex
-scale, starting reference scale of the rock, along X, Y, and Z axis
-scaleDelta, how much the scale can vary from the reference scale
-percentageOfMineralVertexes, from 0 to 1, indicates the percentage of vertexes that will get a mineral color instead of the standard one.
-minerals, what minerals is possible to find in the rock, for every mineral we have:
	-lerp, from 0 to 1, how much the mineral color will blend with the rock color
	-lerpDelta, how much lerp can vary from the reference value
	-color, mineral color
-collide, if true the rock will collide with players and other physical entities
THIS 4 PARAMETERS SHOULD BE USED ONLY FOR "SMALL" ROCKS, AS THEY WON'T HAVE A "PERFECT" PHYSICAL BOUNDING BOX, WHEN COLLIDES IS false
-renderingRocksCount, how many rocks will be rendered for every "entity"
-renderingRocksDelta, how much renderingRocksCount can vary
-renderingRocksOffset, how much any rendered rock can vary from the "entity" position
-scaleRandomness, how much the scale can change it's scale for every rendered rock


Procedural Plant generation has been implemented following the awesome paper by Weber And Pen (https://www2.cs.duke.edu/courses/cps124/fall01/resources/p119-weber.pdf), and the dissertation by Charlie Hewitt (https://chewitt.me/Papers/CTH-Dissertation-2017.pdf) 
Editor Plant param List:

See the Weber and Pen paper for a complete explanation of all the parameters.

-shape, how the first level branches length will be distributed (Conical, Spherical, etc)
-growingCoeff, how quickly the plant will grow: with a value of 1.0 it will take 1000 seconds for a "segment" of the plant to reach it's maximum length 
-plantCount, how many plant will be rendered for the same "entity"
-plantCountV, how much plantCount can vary
-plantOffsetV, how much the position can vary relative to the "entity" position
-plantAngleZV, how much the angle can vary relative to the "reference" angle
-attractionUp, how much the plant stems will tend to go toward the sky
-levels, maximum numbers of recursive levels for this kind of plant
-baseSize
-scale,
-scaleV,
-scale_0,
-scaleV_0,
-ratio,
-ratioPower,
-flare,
leafColor,
leafColorV,
leafDimSpeed,
leafOffsetSpeed,
leafScale,
leafScaleV,
leafOffsetV,
leafAngleV,
trunkColorV,
lobeDepth,
lobes,
leafName,
trunkName

-levelParams, for each level:
	-curveRes
	-curveBack
	-curve
	-curveV,
	-segSplits,
	-baseSplits,
	-splitAngle,
	-splitAngleV,
	branches,
	-branchesV,
	-downAngle,
	-downAngleV,
	-rotate,
	-rotateV,
	-lengthCoeff,
	-lengthCoeffV,
	-taper,
	-radiousMod,
	-clonePercRatio,
	-clonePercRatioV,
	baseYoungColor,
	-topYoungColor,
	-baseOldColor,
	-topOldColor,
	radiousIncreaseSpeed,
	-lengthIncreaseSpeed,
	leafCount,
	allLeafsAtStemLength
# notreallyzelda

Was having fun with maths!!!!!
Unreal Engine 5

This is a recently-started project in which I had the goal in mind to work with line traces and some 3d math to create something that felt fun to mess around with in-game!

The provided Character.cpp file contains six functions written in unreal engine's character.cpp template (with the base unreal code ommitted).

The code I added allows the player to:
- shoot a "bullet" (line trace) from the player that can ricochet against surfaces once.
- use "explode attack" which shoots "spikes" (line traces) from the player's location and then launches the player in a direction, explained below.

mechanic breakdowns:
- Bullet:
  - Pretty much just a line trace that ricochets if it hits something. original inspiration was bow, but a line trace is easier to get setup but resembles a gun more in my opinion.
  - Ricochet uses the reflection vector formula (which i googled, i most definitely didn't have it memorized). I discovered that unreal has their own function for it (FMath::GetReflectionVector) shortly after , but decided to just keep what I made.
  - The ricocheted line trace length is the remaining distance the bullet had to travel before it ricocheted.
 
- Explode attack:
  - The original goal was to create an ability that the player could take into a warring battlefield and use in the midst of battle to damage surrounding enemies simultaneously. but it ended up being that + a launchpad because it was funny.
  - The ExplodeAttack() function calls the necessary functions in succession and then checks if it can launch the player:
    - Randomly generate # of spikes to shoot out (in a real game, this could be determined by power level, other stats, or even # of surrounding enemies)
    - Find the locations where the spikes will shoot from the player, using another fancy math thing called Fibonacci Lattice to evenly distribute the spots in a sphere shape. Again, I did not invent the Fibonacci Lattice or anything else starting with Fibonacci.
    - "Shoot" (line trace) the spikes and record if any of them hit. return those.
    - Randomly get 3 of those hits to create a plane which then the player is launched in the direction of the plane's normal. (bc it was funny idk) later realized it would be cool if those 3 hits were instead the closest ones so the player had some agency over the direction they launched, but haven't implemented yet.
   

Thanks for reading! If you have any questions, lmk. It's only ~180 lines of code, but I thought this was cool so I decided to show it off. I didn't have anything longer to show off anyways!

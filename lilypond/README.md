The `attempt1/sim.py` is a python script that allows us to simulate a Poisson Lilypond model.
There is a small bug in attempt1 that was never squashed. Sometimes we end up with a grain that
doesn't get evaluated meaning the animation (and probably much more) is made incorrect.

Currently the code runs in n^4 time where n is the number of grains created.

I have introduced a few new elements to the model.
* It offers the ability to provide a non-homogeneous Poisson Point process (Using thinning)
* Offers growths of non homogeneous growth rates
* Ability to produce grains external from the initial time
  * This is also known as random sequential adsorption

For more information please look at the `report.tex`

# to do
Focus on better algorithms and where the bottlenecks might be.

Maybe reduce to C.

Create two realisations of the same set.
Given the points and they have same growth rates vs not
They have same birth times vs not
(This is very easy since we create our complex model then make copies
where we force all velocities to the same or birth rates)

How will we extend to 3d?

Prove percolation?
Suppose we either expand all the grains after everyone has stopped growing
and denote a tolerance range (we want percolation of a certain size)
Another question is given that balls appear across many times could this
model percolate from within (a set of connecting balls that just continue forever)

Maximum radius?

Total volume?

Largest connected component?
We measure "largest" by the same method as Last and Penrose (2010)
* Volume
* Metric diameter
* Number of constituent grains

How can we force clustering? What can we change to make clustering more
likely and even the potential for a single large connected component?
Could this component extend to infinity and with what probability.

What if we use different shapes? A square would be pretty easy.

A discrete alternative to the Poisson Lilypond model?
Each grain grows to the 4/8 neighbour pixels.
Maybe we grow the grains with respect to a Manhattan distance?

Actually write report.

Understand why the thinning method is working

attempt2 will be written in C and various techniques will be used to
reduce the constant factor for our time. This will hopefully evaluation faster
for small samples.
The C does not create an animation but takes in a plain text file and outputs
a plain text file of the same format that
contains tuples of each grain, position, birth time, growth rate,
final radius and which grain it collided with (helpful for generating
the graph). The C program overrides the radius.
From there a visualised (made in python) can be used
to parse and visualise our result.

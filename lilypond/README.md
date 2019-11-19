The sim.py is a python script that allows us to simulate a Poisson Lilypond model.

Currently the code runs in n^2 time where n is the number of grains created.

I have introduced a few new elements to the model.
* It offers the ability to provide a non-homogeneous Poisson Point process (Using thinning)
* Offers growths of non homogeneous growth rates
* Ability to produce grains external from the initial time (WIP)
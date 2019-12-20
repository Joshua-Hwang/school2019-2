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

# todo
* Next thing to do is update `approxlp.py` and `solvelp` to include the collisions
section and the dimensions section.
  * First we need to implement parsing dimensions.
    * If dimensions are not defined then we continue the no boundary case
    * If they are defined we need to ensure all points reside within the boundaries
  * We need to modify the distance formula since we need to consider
  the looping case and the hard boundary case
  * If we're doing the hard boundary we need to replace the spair with
  the closest boundary. The paired grain will be the NULL ptr.
    * Every function now needs to handle g2 == NULL

* Afterwards begin documenting EVERYTHING

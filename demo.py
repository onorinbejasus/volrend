import volrend
import cyclops

vr = volrend.initialize()
vr.loadTiff('volrend/divergence.tif')

# move camera back a bit
getDefaultCamera().setBackgroundColor(Color(0,0,0,0))
getDefaultCamera().translate(Vector3(0, 0, 20), Space.Local)
getDefaultCamera().getController().setSpeed(10)

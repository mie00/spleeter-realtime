
import tensorflow as tf

if not tf.test.is_gpu_available():
    raise Exception("gpu unavailable, go here for instructions on how to run tensorflow with gpu https://www.tensorflow.org/install/gpu")

print("GPU found")

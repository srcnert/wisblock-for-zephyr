Overview
********

This sample application records 6 seconds of audio from the RAK18030 PDM microphone
at 16 kHz, mono, 16-bit. The raw PCM samples are printed to the console as
comma-separated values.

A helper Python script (``script/make_wav.py``) is included to convert the
console output into a WAV file for playback.

References
**********

 - `RAK18030 sensor <https://store.rakwireless.com/products/rak18030/>`_

Building and Running
********************

This project captures audio from the RAK18030 PDM microphone and outputs raw
PCM data to the console. It requires a RAK18030 module connected to the
RAK19007 base board.

Building for rak4631
--------------------

:zephyr:board:`rak4631` as follows:

.. zephyr-app-commands::
   :zephyr-app: app/sensor/rak18030
   :board: rak4631
   :goals: build flash
   :west-args: --no-sysbuild

Sample Output
=============

.. code-block:: console

   DMIC sample: rak4631
   Waiting for mic to stabilize...
   PCM output rate: 16000, channels: 1
   Start of mic record
   -12,45,-30,67,120,-89,34,56,-23,78,90,-45,12,34,-56,78,
   ...
   End of mic record
   Exiting

Converting to WAV
-----------------

Copy the comma-separated PCM data from the console output and paste it into the
``raw_data_block`` variable in ``script/make_wav.py``, then run:

.. code-block:: console

   python script/make_wav.py

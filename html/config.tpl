<!DOCTYPE html>
<html>
<head><title>Esp8266 web server</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
<div id="main">
<h1>Thing Configuration</h1>
<form method="post" action="/config.cgi">

<table>
<tr><td colspan=2><h3>ADC Functions</h3></td></tr>
<tr><td><label for="ADCOn">ADC Enabled</label></td><td><input type="checkbox" id="ADCOn" name="ADCOn" %ADCOn%></td></tr>
<tr><td><label for="ADCSerialOutputOn">Output ADC data to serial port</label></td><td><input type="checkbox" id="ADCSerialOutputOn" name="ADCSerialOutputOn" %ADCSerialOutputOn%></td></tr>
<tr><td><label for="ADCDecoderOutputOn">Output ADC to Decoder</label></td><td><input type="checkbox" id="ADCDecoderOutputOn" name="ADCDecoderOutputOn" %ADCDecoderOutputOn%></td></tr>
<tr><td><label for="ADCSpinDetectionOn">Enable spin detection (for power meters)</label></td><td><input type="checkbox" id="ADCSpinDetectionOn" name="ADCSpinDetectionOn" %ADCSpinDetectionOn%></td></tr>

<tr><td><input type="reset" value="Reset"></td><td><input type="submit" name="submit" value="Save"></td></tr>
</table>

</form>
</div>
</body></html>

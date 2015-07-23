<!DOCTYPE html>
<html>
<head><title>Esp8266 web server</title>
<link rel="stylesheet" type="text/css" href="style.css">
<style>
.inline-block{
display: inline-block
}
</style>
</head>
<body>
<div id="main">
<h1>Thing Configuration</h1>
<form method="post" action="/config.cgi">
<table>
<tr><td colspan=2><h3>ADC Functions</h3></td></tr>
<tr><td><label for="ADCOn">ADC Enabled</label></td><td><input type="checkbox" id="ADCOn" name="ADCOn" %ADCOn%></td></tr>
<tr><td><label for="ADCSerialOutputOn">Output ADC data to serial port</label></td><td><input type="checkbox" id="ADCSerialOutputOn" name="ADCSerialOutputOn" %ADCSerialOutputOn%></td></tr>
<tr><td><label for="ADCDecoderOutputOn">Output ADC to Decoder</label></td><td><input type="checkbox" id="ADCDecoderOutputOn" name="ADCDecoderOutputOn" %ADCDecoderOutputOn%>
	<div class="inline-block"><label for=bit0>bit0</label><br><select id=bit0 name=bit0></select></div>
	<div class="inline-block"><label for=bit1>bit1</label><br><select id=bit1 name=bit1></select></div>
	<div class="inline-block"><label for=bit2>bit2</label><br><select id=bit2 name=bit2></select></div>
</td></tr>
<tr><td><label for="ADCSpinDetectionOn">Enable spin detection (for power meters)</label></td><td><input type="checkbox" id="ADCSpinDetectionOn" name="ADCSpinDetectionOn" %ADCSpinDetectionOn%></td></tr>
<tr><td><label for="ADCChannelHost">IP address of thingspeak.com or other service</label></td><td><input type="text" name="ADCChannelHost" id="ADCChannelHost" value="%ADCChannelHost%" maxlength="63"></td></tr>
<tr><td><label for="ADCChannelPayload">Channel URI in c printf format</label><br><small>Example: /update?key=&#37;&amp;field1=&#37;d<br>api key must be the first parameter, data - second one</small></td><td><input type="text" name="ADCChannelPayload" id="ADCChannelPayload" value="%ADCChannelPayload%" maxlength=255 size=80></td></tr>
<tr><td><label for="ADCChannelAPIKey">API Key</label></td><td><input type="text" name="ADCChannelAPIKey" id="ADCChannelAPIKey" value="%ADCChannelAPIKey%" maxlength="32"></td></tr>
<tr><td><input type="reset" value="Reset"></td><td><input type="submit" name="submit" value="Save"></td></tr>
</table>
</form>
</div>
</body>
<script type="text/javascript">
function id(s){return document.getElementById(s)}
gpioOptions(id('bit0'), %DecoderOutputBit0%);
gpioOptions(id('bit1'), %DecoderOutputBit1%);
gpioOptions(id('bit2'), %DecoderOutputBit2%);
function gpioOptions(el, selected){
	var op;
	for (var i=0;i<16;i++){
		op=document.createElement('option');
		op.textContent='GPIO'+i;
		op.setAttribute('value', i);
		if (i==selected){
			op.setAttribute('selected', '');			
		}
		el.appendChild(op);
	}
}
</script>
</html>

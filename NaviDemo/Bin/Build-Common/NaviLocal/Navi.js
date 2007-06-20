function PageNaviData()
{
	var url = window.document.URL.toString();
	
	this.getName = getName;
	this.doesExist = doesExist;
	this.get = get;
	
	function getName()
	{
		if(url.indexOf("?") > -1 && url.indexOf("?", url.indexOf("?")+1) > -1)
			return url.substr(url.indexOf("?")+1).split("?")[0];

		return "";
	}
	
	function doesExist(ParameterName)
	{
		if(get(ParameterName).length)
			return true;
		else
			return false;
	}
		
	function get(ParameterName)
	{
		if(url.indexOf("?") > -1 && url.indexOf("?", url.indexOf("?")+1) > -1)
		{
			var Parameters = url.substr(url.indexOf("?", url.indexOf("?")+1)+1).split("&");
			for (i = 0; i < Parameters.length; i++)
			{
				if(Parameters[i].indexOf("=") > 0)
				{
					var ParameterValue = Parameters[i].split("=");
					if(ParameterValue[0] == ParameterName) 
						return decodeString(ParameterValue[1]);
				}
			}
		}
		return "";
	}
}

pageNaviData = new PageNaviData();

function NaviData(naviDataName)
{
	var dataString = "NAVI_DATA:?" + naviDataName + "?";
	var paramCount = 0;
	this.add = add;
	this.send = send;
	
	function add(paramName, paramValue)
	{
		if(paramCount != 0)
			dataString += "&";
		
		if(isNaN(paramValue))
			dataString += paramName + "=" + encodeString(paramValue);
		else
			dataString += paramName + "=" + paramValue;
			
		paramCount++;
	}
	
	function send()
	{
		window.statusbar = false;
		window.status = dataString;
	}
}

function decodeString(strtod)
{
   strtod = strtod.replace(/%20%20/g, " &nbsp;");
   strtod = strtod.replace(/%20/g, " ");
   strtod = strtod.replace(/%22/g, "\"");
   strtod = strtod.replace(/%60/g, "`");
   strtod = strtod.replace(/%5C/g, "\\");
   strtod = strtod.replace(/%23/g, "#");
   strtod = strtod.replace(/%3C/g, "<");
   strtod = strtod.replace(/%3E/g, ">");
   strtod = strtod.replace(/%26/g, "&");
   strtod = strtod.replace(/%3D/g, "=");
   strtod = strtod.replace(/%3F/g, "?");
   strtod = strtod.replace(/%25/g, "%");
   
   return strtod;
}

function encodeString(strtoe)
{
	strtoe = strtoe.replace(/%/g, "%25");
	strtoe = strtoe.replace(/&nbsp;/g, "%20");
	strtoe = strtoe.replace(/ /g, "%20");
	strtoe = strtoe.replace(/"/g, "%22");
	strtoe = strtoe.replace(/`/g, "%60");
	strtoe = strtoe.replace(/\\/g, "%5C");
	strtoe = strtoe.replace(/#/g, "%23");
	strtoe = strtoe.replace(/</g, "%3C");
	strtoe = strtoe.replace(/>/g, "%3E");
	strtoe = strtoe.replace(/&/g, "%26");
	strtoe = strtoe.replace(/=/g, "%3D");
	strtoe = strtoe.replace(/\?/g, "%3F");
	
	return strtoe;
}
	
function getTime()
{
	var suffix = "";
	var now = new Date();
	var hours = now.getHours();
	
	if (hours < 12)
		suffix = "AM";
	else
		suffix = "PM";
	
	if (hours == 0)
		hours = 12;
		
	if (hours > 12)
		hours = hours - 12;

	var minutes = String(now.getMinutes());

	if (minutes.length == 1)
		minutes = "0" + minutes;

	return String(String(hours) + ":" + minutes + " " + suffix);
}

function getNaviName()
{
	return navigator.userAgent.substr(navigator.userAgent.lastIndexOf(" "));
}
<!DOCTYPE html
	PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
	 "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US">
<head>
<title>chmod plive</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
</head>
<body>
<p align="center"><img src="/images/NRC_banner_e.jpg" /></p>

<p>PortageLive server maintenance script. Install, tweak and use as needed, but
only if you really know what you're doing!</p>

<?php
  $cmd = "chmod 777 /var/www/html/plive/*";
  print "Running $cmd <br/>";
  print `$cmd 2>&1 | sed 's/$/<br\/>/'`;
?>

<hr/>
<table width="100%" cellspacing="0" cellpadding="0" border="0">
   <tr>
      <td width="20%" align="right" valign="center">
	 <img alt="NRC-ICT" src="images/sidenav_graphic.png" height="44" />
      </td>
      <td width="60%" align="center" valign="bottom">
	 <img width="286" alt="National Research Council Canada" src="/images/mainf1.gif" height="44" />
      </td>
      <td width="20%" align="left" valign="center">
	 <img width="93" alt="Government of Canada" src="/images/mainWordmark.gif" height="44" />
      </td>
   </tr>
   <tr>
      <td>
      </td>
      <td align="center" valign="top">
	 <small>Traitement multilingue de textes / Multilingual Text Processing  <br /> Technologies de l'information et des communications / Information and Communications Technologies <br /> Conseil national de recherches Canada / National Research Council Canada <br /> Copyright 2014, Sa Majest&eacute; la Reine du Chef du Canada /  Her Majesty in Right of Canada <br /> <a href="/portage_notices.php">Third party Copyright notices</a>
	 </small>
      </td>
   </tr>
</table>

</body>
</html>

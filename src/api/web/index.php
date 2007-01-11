<?php
	//define ("__PADDLE_DEBUG__", "1");

        //Always declare our content-type and charset in the HTTP header
        header("Content-type: text/html; charset=utf-8");

	//We need to reset post to make sure we are getting the latest state of the post variables...
	reset($_POST);

	if (defined("__PADDLE_DEBUG__")){
		// Active assert and make it quiet
		assert_options (ASSERT_ACTIVE, 1);
		assert_options (ASSERT_WARNING, 0);
		assert_options (ASSERT_QUIET_EVAL, 1);
	}
	else{
		assert_options (ASSERT_ACTIVE, 0);
	}

	// Set up the callback
	assert_options (ASSERT_CALLBACK, 'ILT_ASSERT');

	//Find out how we were called
	if(count ($_POST) == 0){
		//***INITIAL*** we were directly called without posting anything or using get request (get not supported)
		PrintLangSelection();
	}
	else{
		//***POST*** We were called from a post request
		assert(LoadResources(GetCurrentLanguage()));

		switch($_POST["Screen"]){
			case 0:
				//***We came from the language selection screen***
				PrintInterface();
				break;
			case 1:
				//***We want to translate free text***
				FreeTextTranslate();
				break;
			case 2:
				//***We want to translate fetched text***
				FetchAndTranslate();
				break;
			default:
				assert("FALSE");
		}

		if (defined("__PADDLE_DEBUG__")) DumpPostVars();
		if (defined("__PADDLE_DEBUG__")) DumpResource();
	}
?>




<?php
function DumpResource(){
	if (defined("__LOGO_REF__")){
?>
<hr>
<h1>Defined Constants: </h1>
__LOGO_REF__ : <?echo __LOGO_REF__?> <br>
__EMPTY_TEXT_AREA_WARNING__ : <?echo __EMPTY_TEXT_AREA_WARNING__?> <br>
__SWITCH_LANG_TEXT__ : <?echo __SWITCH_LANG_TEXT__?> <br>
__SWITCH_LANG_REF__ : <?echo __SWITCH_LANG_REF__?> <br>
__TEXT_BOX_HEADER__ : <?echo __TEXT_BOX_HEADER__?> <br>
__RADIO_LABEL_DIR_FR_EN__ : <?echo __RADIO_LABEL_DIR_FR_EN__?> <br>
__RADIO_LABEL_DIR_EN_FR__ : <?echo __RADIO_LABEL_DIR_EN_FR__?> <br>
__RADIO_LABEL_DIR_CH_EN__ : <?echo __RADIO_LABEL_DIR_CH_EN__?> <br>
__BTN_LABEL_SUBMIT__ : <?echo __BTN_LABEL_SUBMIT__?> <br>
__BTN_LABEL_RESET__ : <?echo __BTN_LABEL_RESET__?> <br>
__SITES_LIST_HEADER__ : <?echo __SITES_LIST_HEADER__?> <br>
__SITE_A_VALUE__ : <?echo __SITE_A_VALUE__?> <br>
__SITE_A_HREF__ : <?echo __SITE_A_HREF__?> <br>
__SITE_A_TEXT__ : <?echo __SITE_A_TEXT__?> <br>
__SITE_B_VALUE__ : <?echo __SITE_B_VALUE__?> <br>
__SITE_B_HREF__ : <?echo __SITE_B_HREF__?> <br>
__SITE_B_TEXT__ : <?echo __SITE_B_TEXT__?> <br>
__SITE_C_VALUE__ : <?echo __SITE_C_VALUE__?> <br>
__SITE_C_HREF__ : <?echo __SITE_C_HREF__?> <br>
__SITE_C_TEXT__ : <?echo __SITE_C_TEXT__?> <br>
__SITE_D_VALUE__ : <?echo __SITE_D_VALUE__?> <br>
__SITE_D_HREF__ : <?echo __SITE_D_HREF__?> <br>
__SITE_D_TEXT__ : <?echo __SITE_D_TEXT__?> <br>
__SITE_E_VALUE__ : <?echo __SITE_E_VALUE__?> <br>
__SITE_E_HREF__ : <?echo __SITE_E_HREF__?> <br>
__SITE_E_TEXT__ : <?echo __SITE_E_TEXT__?> <br>
__SITE_F_VALUE__ : <?echo __SITE_F_VALUE__?> <br>
__SITE_F_HREF__ : <?echo __SITE_F_HREF__?> <br>
__SITE_F_TEXT__ : <?echo __SITE_F_TEXT__?> <br>
__SITE_G_VALUE__ : <?echo __SITE_G_VALUE__?> <br>
__SITE_G_HREF__ : <?echo __SITE_G_HREF__?> <br>
__SITE_G_TEXT__ : <?echo __SITE_G_TEXT__?> <br>
__SITE_H_VALUE__ : <?echo __SITE_H_VALUE__?> <br>
__SITE_H_HREF__ : <?echo __SITE_H_HREF__?> <br>
__SITE_H_TEXT__ : <?echo __SITE_H_TEXT__?> <br>
__SITE_I_VALUE__ : <?echo __SITE_I_VALUE__?> <br>
__SITE_I_HREF__ : <?echo __SITE_I_HREF__?> <br>
__SITE_I_TEXT__ : <?echo __SITE_I_TEXT__?> <br>
__SITE_J_VALUE__ : <?echo __SITE_J_VALUE__?> <br>
__SITE_J_HREF__ : <?echo __SITE_J_HREF__?> <br>
__SITE_J_TEXT__ : <?echo __SITE_J_TEXT__?> <br>
__TRANSLATOR_ENGINE_NAME__ : <?echo __TRANSLATOR_ENGINE_NAME__?> <br>
__COPYRIGHTS_NOTE__ : <?echo __COPYRIGHTS_NOTE__?> <br>
__NEW_TRANSLATION_BTN__ : <?echo __NEW_TRANSLATION_BTN__?> <br>
__TRANSLATION_RESULT_TITLE__ : <?echo __TRANSLATION_RESULT_TITLE__?> <br>
__TRANSLATION_RESULT_SRC_TITLE__ : <?echo __TRANSLATION_RESULT_SRC_TITLE__?> <br>
__TRANSLATION_RESULT_TGT_TITLE__ : <?echo __TRANSLATION_RESULT_TGT_TITLE__?> <br>
<hr>
<?
	}
}

function DumpPostVars(){
	echo "<hr>";
	echo "<h1>Post Variables</h1>";

	while(list($key, $value) = each($_POST)){
		echo "$key = $value<br>\n";
	}
	echo "<hr>";
}

function ILT_ASSERT ($file, $line, $code) {
	echo "<hr>Assertion Failed: File '$file'<br>
        Line '$line'<br>
        Code '$code'<br><hr>";
}

function GetCurrentLanguage(){
	$retv = "FALSE";

	$retv = $_POST["btn_language"];
	if($retv == ""){
		$retv = $_POST["Language"];
	}

	return $retv;
}

function LoadResources($language){
	$retv = "FALSE";
	switch($language){
		case "Français":
			require_once("fr_res.php");
			$retv = "TRUE";
			break;
		case "English":
			require_once("en_res.php");
			$retv = "TRUE";
			break;
	 }
	 return $retv;
}

function PrintLangSelection(){
$me = $_SERVER['PHP_SELF'];

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<title>Portage Online</title>
</head>
<body bgcolor="#ffffff">
	<form action="index.php" method="post">
	<table border="0" width="100%" height="100%">
		<tr>
			<td>
				<div align="center">
				<table border="0">
					<tr>
						<td><p align="center"><a href="http://www.crnc-nrc.gc.ca" target="_self"><img src="corprt_splash_clf_07.jpg" border="0"></a></td>
					</tr>
					<tr>
						<td>
						<table border="0" width="100%">
							<tr>
								<td align="center"><INPUT TYPE="submit" name="btn_language" VALUE="Français"></td>
								<td align="center"><INPUT TYPE="submit" name="btn_language" VALUE="English"></td>
							</tr>
						</table>
						</td>
					</tr>
				</table>
				</div>
			</td>
		</tr>
	</table>
	<INPUT TYPE="hidden" name="Language" value="">
	<INPUT TYPE="hidden" name="Screen" Value="0">
	</FORM>
</body>
</html>
<?
}

function PrintInterface(){
$me = $_SERVER['PHP_SELF'];

?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title><?echo __TRANSLATOR_ENGINE_NAME__?></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">

<script language="JavaScript" type="text/javascript">
	function isValid(){
 		var texte=document.formulaire.text.value;
		if (!texte){
	 		// something else is wrong
	 		alert('<?echo __EMPTY_TEXT_AREA_WARNING__?>');
	 		return false;
 		}
 	// If the script makes it to here, everything is OK,
 	// so you can submit the form

 	return true;
	}
</script>

</head>
<body>
<center>
<table width="40%" border="0" cellspacing="0" cellpadding="0">
    <tr>
      <td>
  		<a href="http://www.cnrc-nrc.gc.ca/" target="_self"><img src="<?echo __LOGO_REF__?>" border = "0" alt="CNRC-NRC"></a>
  	</td>
    </tr>
    <tr>

      <td align="right">
      	<form action="<?=$me?>" method="post">
			<INPUT TYPE="submit" name="btn_language" VALUE="<?echo __SWITCH_LANG_TEXT__?>">
			<INPUT TYPE="hidden" name="Language" value="">
			<INPUT TYPE="hidden" name="Screen" Value="0">
		</FORM>
      </a></td>
    </tr>
  </table>
  <h3><strong><?echo __TEXT_BOX_HEADER__?></strong></h3>

  <table width="40%" border="0" cellspacing="0" cellpadding="5">
  <tr>
    <td>
    <form action="<?=$me?>" method="post" name="formulaire" target="_self" onSubmit = "return isValid()">
    <div align="center">
            <textarea name="text" cols="50" rows="5"></textarea>
    </div><br>
    <div align="center">
    		<input type="radio" name="Menu" checked value="<?echo __RADIO_VALUE_DIR_FR_EN__?>"><?echo __RADIO_LABEL_DIR_FR_EN__?>&nbsp;&nbsp;
			<input type="radio" name="Menu" value="<?echo __RADIO_VALUE_DIR_EN_FR__?>"><?echo __RADIO_LABEL_DIR_EN_FR__?>&nbsp;&nbsp;
			<input type="radio" name="Menu" value="<?echo __RADIO_VALUE_DIR_CH_EN__?>"><?echo __RADIO_LABEL_DIR_CH_EN__?>
	</div>
	<br>
	<div align="center">
            <input name="trad" type="submit" value="<?echo __BTN_LABEL_SUBMIT__?>">&nbsp;
			<input name="reset" type="reset" value="<?echo __BTN_LABEL_RESET__?>">
			<INPUT TYPE="hidden" name="Language" value="<?echo $_POST["btn_language"]?>">
			<INPUT TYPE="hidden" name="Screen" Value="1">
    </div>
  	</form>
	</td>
  </tr>

  <tr><td><hr></td></tr>
  <tr>
   	<td align="left">
   	<form action="<?=$me?>" method="post" name="formulaireChinois" target="_self">
    	<h3 align="center"><strong><?echo __SITES_LIST_HEADER__?></strong></h3>
  			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_A_VALUE__?>"><a href="http://<?echo __SITE_A_HREF__?>" target="_new"><?echo __SITE_A_TEXT__?></a><br>
  			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_B_VALUE__?>"><a href="http://<?echo __SITE_B_HREF__?>" target="_new"><?echo __SITE_B_TEXT__?></a><br>
  			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_C_VALUE__?>"><a href="http://<?echo __SITE_C_HREF__?>" target="_new"><?echo __SITE_C_TEXT__?></a><br>
  			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_D_VALUE__?>"><a href="http://<?echo __SITE_D_HREF__?>" target="_new"><?echo __SITE_D_TEXT__?></a><br>
  			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_E_VALUE__?>"><a href="http://<?echo __SITE_E_HREF__?>" target="_new"><?echo __SITE_E_TEXT__?></a><br>
  			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_F_VALUE__?>" checked="checked"><a href="<?echo __SITE_F_HREF__?>" target="_new"><?echo __SITE_F_TEXT__?></a><br>
  			<br>
			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_G_VALUE__?>"><a href="http://<?echo __SITE_G_HREF__?>" target="_new"><?echo __SITE_G_TEXT__?></a><br>
			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_H_VALUE__?>"><a href="http://<?echo __SITE_H_HREF__?>" target="_new"><?echo __SITE_H_TEXT__?></a><br>
			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_I_VALUE__?>"><a href="http://<?echo __SITE_I_HREF__?>" target="_new"><?echo __SITE_I_TEXT__?></a><br>
			&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" name="Site" value="<?echo __SITE_J_VALUE__?>"><a href="http://<?echo __SITE_J_HREF__?>" target="_new"><?echo __SITE_J_TEXT__?></a><br>

  	<div align="center">
            <input name="trad" type="submit" value="<?echo __BTN_LABEL_SUBMIT__?>">&nbsp;
  			<input name="reset" type="reset" value="<?echo __BTN_LABEL_RESET__?>">
  			<INPUT TYPE="hidden" name="Language" value="<?echo $_POST["btn_language"]?>">
			<INPUT TYPE="hidden" name="Screen" Value="2">
    </div>
    </form>
    </td>
  </tr>

</table>
</center>


  <!-- begin footer; it would be nice if you would leave this on. ;) -->

<center>
<table cellpadding="10">
	<tr>
		<td align="center">
      		<?echo __TRANSLATOR_ENGINE_NAME__?> <br />
      		<?echo __COPYRIGHTS_NOTE__?>
   		</td>
   		<td align="center">
   		<p>
			<a href="http://validator.w3.org/check?uri=referer"><img border="0"
		    src="http://www.w3.org/Icons/valid-html401"
		    alt="Valid HTML 4.01!" height="31" width="88"></a>
    	</p>
   		</td>
  	</tr>
</table>
</center>

 </body>
</html>
<?
}


function FetchAndTranslate(){
	$file = "temp/" . md5(uniqid(rand(), true));

  	//fetch
	switch($_POST["Site"]){
	   case __SITE_A_VALUE__:
	   	 $site_to_fetch = "http://" . __SITE_A_HREF__;
		 $out = exec("wget -O $file $site_to_fetch");
		 break;
       case __SITE_B_VALUE__:
	   	 $site_to_fetch = "http://" . __SITE_B_HREF__;
		 $out = exec("wget -O $file $site_to_fetch");
		 break;
       case __SITE_C_VALUE__:
	   	 $site_to_fetch = "http://" . __SITE_C_HREF__;
		 $out = exec("wget -O $file $site_to_fetch");
		 break;
       case __SITE_D_VALUE__:
	   	 $site_to_fetch = "http://" . __SITE_D_HREF__;
		 $out = exec("wget -O $file $site_to_fetch");
		 break;
       case __SITE_E_VALUE__:
	   	 $site_to_fetch = "http://" . __SITE_E_HREF__;
		 $out = exec("wget -O $file $site_to_fetch");
		 break;
	   case __SITE_F_VALUE__:
	     $file_to_fetch = __SITE_F_HREF__;
		 $out = exec("cp $file_to_fetch $file");
		 break;
       case __SITE_G_VALUE__:
	   	 $site_to_fetch = "http://" . __SITE_G_HREF__;
		 $out = exec("wget -O $file $site_to_fetch");
		 break;
       case __SITE_H_VALUE__:
	   	 $site_to_fetch = "http://" . __SITE_H_HREF__;
		 $out = exec("wget -O $file $site_to_fetch");
		 break;
       case __SITE_I_VALUE__:
	   	 $site_to_fetch = "http://" . __SITE_I_HREF__;
		 $out = exec("wget -O $file $site_to_fetch");
		 break;
       case __SITE_J_VALUE__:
	   	 $site_to_fetch = "http://" . __SITE_J_HREF__;
		 $out = exec("wget -O $file $site_to_fetch");
		 break;
	}

	//translate
	require_once("constants.php");

	//Needed to be able to execute the client
	$PORTAGE = "/export/projets/portage/i686-g3.4.4";
	putenv("PORTAGE=$PORTAGE");
	putenv("LD_LIBRARY_PATH=$PORTAGE/lib");

	$cmd = "./" . __TRANSLATION_CLIENT_NAME__ . " " . $file . " " . __CH_2_EN_SERVER_PORT_NUM__ . " " . __CH_2_EN_SERVER_HOST__;

	$answer_from_client = `$cmd`;

	//Display
	$me = $_SERVER['PHP_SELF'];
?>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>-- <?echo __TRANSLATION_RESULT_TITLE__?> --</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>

<body>
<div>
<center>
<p><a href="http://www.cnrc-nrc.gc.ca/" target="_self"><img src="<?echo __LOGO_REF__?>" border = "0" alt="CNRC-NRC"></a></p>
<p>&nbsp;</p>
<h3><strong><?echo __TRANSLATION_RESULT_TITLE__ . " :"?></strong></h3>
</center>
</div>
<?echo $answer_from_client?>
<center>
<br>



<hr>
<input type="Button" value="<?echo __NEW_TRANSLATION_BTN__?>" onclick="history.back()">
<!--
<form action="<?=$me?>" method="post">
<input name="trad" type="submit" value="<?echo __NEW_TRANSLATION_BTN__?>">
<INPUT TYPE="hidden" name="Language" value="<?echo $_POST["Language"]?>">
<INPUT TYPE="hidden" name="Screen" Value="0">
</form>
-->
<br>
<br>
<table cellpadding="10">
	<tr>
		<td align="center">
      		<?echo __TRANSLATOR_ENGINE_NAME__?> <br />
      		<?echo __COPYRIGHTS_NOTE__?>
   		</td>
   		<td align="center">
   		<p>
			<a href="http://validator.w3.org/check?uri=referer"><img border="0"
		    src="http://www.w3.org/Icons/valid-html401"
		    alt="Valid HTML 4.01!" height="31" width="88"></a>
    	</p>
   		</td>
  	</tr>
</table>
</center>

</body>
</html>
<?

	//cleanup
	$rm_retv = `rm $file`;
}

function FreeTextTranslate(){
	$file = "temp/" . md5(uniqid(rand(), true));

	$text = stripslashes($_POST["text"]);

	//write content to a file
	$out = fopen($file,'w');
	fwrite($out, $text);
	fclose($out);

	//translate
	require_once("constants.php");

	//Needed to be able to execute the client
	$PORTAGE = "/export/projets/portage/i686-g3.4.4";
	putenv("PORTAGE=$PORTAGE");
	putenv("LD_LIBRARY_PATH=$PORTAGE/lib");

	switch($_POST["Menu"]){
		case __RADIO_VALUE_DIR_FR_EN__:
			//pre portage conversion from utf-8 to iso-8859-1
			`cat $file | iconv -f utf-8 -t iso-8859-1 > $file.iso-8859-1`;
			$cmd = "./" . __TRANSLATION_CLIENT_NAME__ . " " . $file . ".iso-8859-1 " . __FR_2_EN_SERVER_PORT_NUM__ . " " . __FR_2_EN_SERVER_HOST__ . " 2>&1 | iconv -f iso-8859-1 -t utf-8";
			//post portage conversion back to utf8 for proper displaying on web page						
			$answer_from_client = `$cmd`;

			if (defined("__PADDLE_DEBUG__")) echo "\$cmd=" . $cmd . "<br>";
			if (defined("__PADDLE_DEBUG__")) echo "\$answer_from_client=" . $answer_from_client . "<br>";

			//cleanup
		        //$rm_retv = `rm $file`;
			//$rm_retv = `rm $file.iso-8859-1`;
			break;
		case __RADIO_VALUE_DIR_EN_FR__:
			//pre portage conversion from utf-8 to iso-8859-1
			`cat $file | iconv -f utf-8 -t iso-8859-1 > $file.iso-8859-1`;
			$cmd = "./" . __TRANSLATION_CLIENT_NAME__ . " " . $file . ".iso-8859-1 " . __EN_2_FR_SERVER_PORT_NUM__ . " " . __EN_2_FR_SERVER_HOST__ . " 2>&1 | iconv -f iso-8859-1 -t utf-8";
			//post portage conversion back to utf8 for proper displaying on web page
			$answer_from_client = `$cmd`;

			if (defined("__PADDLE_DEBUG__")) echo "\$cmd=" . $cmd . "<br>";
                        if (defined("__PADDLE_DEBUG__")) echo "\$answer_from_client=" . $answer_from_client . "<br>";
                        
			//cleanup
                        //$rm_retv = `rm $file`;
                        //$rm_retv = `rm $file.iso-8859-1`;
			break;
		case __RADIO_VALUE_DIR_CH_EN__:
			//pre portage conversion from utf-8 to iso-8859-1
                        `cat $file | iconv -f utf-8 -t gb2312 > $file.gb2312`;
			$cmd = "./" . __TRANSLATION_CLIENT_NAME__ . " " . $file . ".gb2312 " . __CH_2_EN_SERVER_PORT_NUM__ . " " . __CH_2_EN_SERVER_HOST__ . " 2>&1 | iconv -f gb2312 -t utf-8";
			//post portage conversion back to utf8 for proper displaying on web page
                        $answer_from_client = `$cmd`;

			if (defined("__PADDLE_DEBUG__")) echo "\$cmd=" . $cmd . "<br>";
                        if (defined("__PADDLE_DEBUG__")) echo "\$answer_from_client=" . $answer_from_client . "<br>";

                        //cleanup
                        //$rm_retv = `rm $file`;
                        //$rm_retv = `rm $file.gb2312`;

			break;
		default:
			assert("FALSE");
	}

	//Display
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>-- <?echo __TRANSLATION_RESULT_TITLE__?> --</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>

<body>
<div>
<center>
  <p><a href="http://www.nrc-cnrc.gc.ca/" target="_self"><img src="<?echo __LOGO_REF__?>" border = "0" alt="CNRC-NRC"></a></p>
  <p>&nbsp;</p>
  <h3><strong><?echo __TRANSLATION_RESULT_SRC_TITLE__?></strong></h3>
</center>
</div>

<div align="left">
	<?echo $text?>
</div>

<br>
<p><h3 align="center"><?echo __TRANSLATION_RESULT_TGT_TITLE__?></h3></p>
<br>

<div align="left">
	<?echo $answer_from_client?>
</div>

<center>
<br>
<hr>
<input type="Button" value="<?echo __NEW_TRANSLATION_BTN__?>" onclick="history.back()">
<!--
<form action="<?php echo $_SERVER['PHP_SELF']; ?>" method="post">
<input name="trad" type="submit" value="<?echo __NEW_TRANSLATION_BTN__?>">
<INPUT TYPE="hidden" name="Language" value="<?echo $_POST["Language"]?>">
<INPUT TYPE="hidden" name="Screen" Value="0">
</form>
-->
<br>
<br>
<table cellpadding="10">
	<tr>
		<td align="center">
      		<?echo __TRANSLATOR_ENGINE_NAME__?> <br />
      		<?echo __COPYRIGHTS_NOTE__?>
   		</td>
   		<td align="center">
   		<p>
			<a href="http://validator.w3.org/check?uri=referer"><img border="0"
		    src="http://www.w3.org/Icons/valid-html401"
		    alt="Valid HTML 4.01!" height="31" width="88"></a>
    	</p>
   		</td>
  	</tr>
</table>
</center>

</body>
</html>
<?

	//cleanup
	//$rm_retv = `rm $file`;
}


?>

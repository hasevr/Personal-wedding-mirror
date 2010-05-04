<?php
date_default_timezone_set("Asia/Tokyo");
echo "This is AddText.php: start!\n";
$dirname = "img";

function AddText($fn, $mess, $pos){
	$mess = trim($mess);
	echo "AddText($fn, ", mb_convert_encoding($mess, "SJIS", "UTF-8"), ");\n";
	global $dirname;
	$im = imagecreatefromjpeg("$dirname/$fn");
	if (!$im){
		echo "Cannot open jpeg file: $fn .\n";
		return;
	}
	$x = imagesx($im);
	$y = imagesy($im);
	
	$size = 28;//24;
	$angle = 0;
//	$font = "/Windows/Fonts/HGRSMP.TTF";
//	$font = "./mika_PB.TTF";
//	$font = "./cinecaption226.ttf";
	$font = "./KFhimaji.otf";

	$lines = explode("\n", $mess);
	$lineH = 0;
	foreach($lines as $line){
		$bbox = ImageTTFBBox($size, $angle, $font, $line);
		$lineH += $bbox[1]-$bbox[7];
	}
	$lineH /= count($lines);	//	per one line
	
	//	行間の設定、表示位置の設定
	$lineH += 0;;
	$margin = $size*1.5;
	if ($pos=="t"){
		$top = $margin-10;
	}else{
		$top = $y-10-$margin - count($lines)*$lineH;
	}
	for($i=0; $i<count($lines); $i++){
		$line = $lines[$i];
		$bbox = ImageTTFBBox($size, $angle, $font, $line);
		$txtW = $bbox[2]-$bbox[0];
		$txtH = $bbox[1]-$bbox[7];
		$txtT = $bbox[7];
		if ($txtW && $txtH){
			echo "w:$txtW h:$txtH\n";
			$tmp = imageCreateTruecolor($txtW+20, $txtH+20);
			imagecopy($tmp, $im, 0, 0, $x/2-$txtW/2-10, $top + $i*$lineH,
				$txtW+20, $txtH+20);
			$color = imagecolorallocateAlpha ($tmp, 0xFF, 0xFF, 0xFF, 0);
			$back  = imagecolorallocateAlpha ($tmp, 0x00, 0x00, 0x00, 90);
			for($dx=-2; $dx<=2; $dx++){
				for($dy=-2; $dy<=2; $dy++){
					imageTTFText($tmp,$size,$angle, 
					10+$dx, 10+$dy-$txtT, $back, $font, $line);
				}
			}
			imageFilter($tmp, IMG_FILTER_GAUSSIAN_BLUR);
			imageTTFText($tmp,$size,$angle,10, 10-$txtT, $color, $font, $line);
			imagecopy($im, $tmp, $x/2-$txtW/2-10, $top + $i*$lineH, 0,0,
				$txtW+20, $txtH+20);
			imageDestroy($tmp);
		}
	}
	imagejpeg($im, "../$fn");
	imageDestroy($im);
}



$txt = file("text.txt");
$mess = "";
$num = 0;
$pos = "b";
for($i=0;; $i++){
	$line=@$txt[$i];
	$find = strpos($line, "\t");
	if (!($find === FALSE) || $i == count($txt)){
		//	次の行に進んだので、前の行を出力
		$n = substr($line, 0, $find);
		$m = substr($line, $find+1);
		$find = strpos($m, "\t");
		$key = substr($m, 0,1);
		$newpos = "b";
		if ($key=="u" || $key=="t" || $key=="d" || $key=="b"){
			if ($key=="u" || $key=="t") $newpos = "t";
			$m = substr($m, $find+1);
		}
		if (is_numeric($n) || $i == count($txt)){
			if ($mess){
				//echo "num:$num mess: ", mb_convert_encoding($mess, "SJIS", "UTF-8");
				$dir = scandir($dirname);
				foreach($dir as $fn){
			 		if (substr($fn,0,2) == $num){
				 		AddText("$fn", $mess, $pos);
				 		break;
				 	}
			 	}
			}
			$num = $n;
			$mess = $m;
			$pos = $newpos;
		}else{
			$mess .= "\n";
			$mess .= $line;
		}
	}else{
	 	$mess .= "\n";
	 	$mess .= $line;
	}
	if ($i == count($txt)) break;
}
?>

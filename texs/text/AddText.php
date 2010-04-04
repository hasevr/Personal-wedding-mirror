<?php
date_default_timezone_set("Asia/Tokyo");
echo "This is AddText.php: start!\n";


function imagecopymerge_alpha($dst_im, $src_im, $dst_x, $dst_y, $src_x, $src_y, $src_w, $src_h, $pct){
        $opacity=$pct;
        // getting the watermark width
        $w = imagesx($src_im);
        // getting the watermark height
        $h = imagesy($src_im);
         
        // creating a cut resource
        $cut = imagecreatetruecolor($src_w, $src_h);
        // copying that section of the background to the cut
        imagecopy($cut, $dst_im, 0, 0, $dst_x, $dst_y, $src_w, $src_h);
        // inverting the opacity
        $opacity = 100 - $opacity;
         
        // placing the watermark now
        imagecopy($cut, $src_im, 0, 0, $src_x, $src_y, $src_w, $src_h);
        imagecopymerge($dst_im, $cut, $dst_x, $dst_y, $src_x, $src_y, $src_w, $src_h, $opacity);
    }


$dirname = "img";
function AddText($fn, $mess){
	$mess = trim($mess);
	echo "AddText($fn, $mess);\n";
	global $dirname;
	$im = imagecreatefromjpeg("$dirname/$fn");
	if (!$im){
		echo "Cannot open jpeg file: $fn .\n";
		return;
	}
	$x = imagesx($im);
	$y = imagesy($im);
	
	$size = 24;
	$angle = 0;
	$font = "/Windows/Fonts/HGRSMP.TTF";
	$bottomMargin = $size*1.5;

	$lines = explode("\n", $mess);
	$lineH = 0;
	foreach($lines as $line){
		$bbox = ImageTTFBBox($size, $angle, $font, $line);
		$lineH += $bbox[1]-$bbox[7];
	}
	$lineH /= count($lines);	//	per one line
	//	行間の設定
	$lineH += 0;	//$size/12;
	for($i=0; $i<count($lines); $i++){
		$line = $lines[$i];
		$bbox = ImageTTFBBox($size, $angle, $font, $line);
		$txtW = $bbox[2]-$bbox[0];
		$txtH = $bbox[1]-$bbox[7];
		$txtT = $bbox[7];
		if ($txtW && $txtH){
			echo "w:$txtW h:$txtH\n";
			$tmp = imageCreateTruecolor($txtW+20, $txtH+20);
			imagecopy($tmp, $im, 0, 0, $x/2-$txtW/2-10, 
				$y - (count($lines)-$i)*$lineH-10 - $bottomMargin, 
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
			imagecopy($im, $tmp, $x/2-$txtW/2-10, 
				$y - (count($lines)-$i)*$lineH-10 - $bottomMargin, 
				0,0, $txtW+20, $txtH+20);
			imageDestroy($tmp);
		}
	}
	imagejpeg($im, "../$fn");
	imageDestroy($im);
}

$txt = file("text.txt");
$mess = "";
$num = 0;
for($i=0;; $i++){
	$line=@$txt[$i];
	$find = strpos($line, "\t");
	if (!($find === FALSE) || $i == count($txt)){
		$n = substr($line, 0, $find);
		$m = substr($line, $find+1);
		if (is_numeric($n) || $i == count($txt)){
			if ($mess){
				echo "num:$num mess: $mess";
				$dir = scandir($dirname);
				foreach($dir as $fn){
			 		if (substr($fn,0,2) == $num){
				 		AddText("$fn", $mess);
				 		break;
				 	}
			 	}
			}
			$num = $n;
			$mess = $m;
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
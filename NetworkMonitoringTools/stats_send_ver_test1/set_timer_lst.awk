BEGIN{
	FS = "[ /,:]+"
	MON = 5
	DAY = 18
	HOUR = var_hour
	MIN = var_min
##SEC < 30
	SEC = 30
}
{
	$4 = MON
	$5 = DAY
	minute = MIN

	if(minute >= 60)
	{
		$7 = minute - 60
		$6 = HOUR + 1
	}
	else
	{
		$7 = minute
		$6 = HOUR
	}

	$8 = SEC + NR
	print $1,$2,$3"/"$4"/"$5","$6":"$7":"$8
}
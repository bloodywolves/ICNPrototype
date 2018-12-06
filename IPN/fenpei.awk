#!/bin/awk -f
BEGIN{
    srand();
}
{
  v = int(rand()*1000000%100+1);
  if(v>0 && v<=30) value =0;
  else if(v>30 && v<=60) value=1;
  else if(v>60 && v<=100) value=2;
  print $1,$2,value"," > "zhuanfabiao226";
}
END{
}

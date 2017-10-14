command:
1，get_card_id
   成功的返回:
     code:0 data:0123456789abcdf message: reply get_card_id successful
   失败的返回：
     code:x ....................

2，set_card_id 8000000000000001
   备注：实际是8字节的card id， 0x80 0x00 0x00 0x00 0x00 0x00 0x00 0x01。当数据
   不满8字节的时候，可以填充0，或者缺省，STB软件会填充。
   成功的返回:
     code:0 data:8000000000000001 message: reply set_card_id successful
   失败的返回：
     code:x ....................

3，get_provider_id
  成功的返回:
     code:0 data:0123456789abcdf message: reply get_provider_id successful
   失败的返回：
     code:x ....................
 

4，set_provider_id 0000000000001118
   同card id的配置方式。
   成功的返回:
     code:0 data:0000000000001118 message: reply set_provider_id successful
   失败的返回：
     code:x ....................


5，get_chip_id
   成功的返回:
     code:0 data:0123456789abcdf message: reply get_chip_id successful
   失败的返回：
     code:x ....................

6，lock_otp
   成功的返回:
     code:0 message: reply lock_otp successful
   失败的返回：
     code:x ....................

【说明】，发送的所有命令都要有‘/n’作为结束符。设置的card id 和 provider id和读出
来的不一样，是通过使用chip id进行3DES加密处理过的。
      

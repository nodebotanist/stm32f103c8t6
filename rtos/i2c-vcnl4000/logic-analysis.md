
Reading the product ID

Start, h26 [ h13 | WR ], h81, 
Restart, h27 [ h13 | RD ], h11 NAK, hFF NAK, Stop

Setting the IR LED

Start, h26 [ h13 | WR ], h83, 
Restart, h27 [ h13 | RD ], h02 NAK, hFF NAK, hFF NAK, hFF NAK, Stop

PROBLEM: DOES NOT RECIEVE 0x14

Set Prox Adjustment

Start, h26 [ h13 | WR ], h8A, 
Restart, h27 [ h13 | RD ], h00 NAK, hFF NAK, Stop

PROBLEM: DOES NOT RECIEVE 0x81

INFINITE LOOP

Start, h26 [ h13 | WR ], h80, 
Restart, h27 [ h13 | RD ], h80 NAK, hFF NAK, Stop
Start, h26 [ h13 | WR ], h80, 
Restart, h27 [ h13 | RD ], h80 NAK, hFF NAK, Stop


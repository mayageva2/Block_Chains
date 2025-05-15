#!/bin/bash

#variables settings
BLOCKS_AMOUNT=$1
URL="https://api.blockcypher.com/v1/btc/main"
CURR_HASH=$(curl -s $URL | awk -F '"' '/"hash":/ {print $4; exit}')
BLOCK_URL="https://api.blockcypher.com/v1/btc/main/blocks"
FILE_NAME="blocks.list"

#creates new file for output
touch $FILE_NAME

#reset output file
>"$FILE_NAME"

for (( i=0; i<BLOCKS_AMOUNT; i++ ))
do
 CURR_BLOCK=$(curl -s $BLOCK_URL/$CURR_HASH)

 #Extract fields
 HASH=$(echo "$CURR_BLOCK"| awk -F '"' '/"hash":/{print $4; exit}')
 HEIGHT=$(echo "$CURR_BLOCK"| grep '"height":'|grep -o '[0-9]\+')
 TOTAL=$(echo "$CURR_BLOCK"| grep '"total":'|grep -o '[0-9]\+')
 TIME=$(echo "$CURR_BLOCK"| awk -F '"' '/"time":/{print $4; exit}')
 RELAYED_BY=$(echo "$CURR_BLOCK"| awk -F '"' '/"relayed_by":/{print $4; exit}')
 PREV_BLOCK=$(echo "$CURR_BLOCK"| awk -F '"' '/"prev_block":/{print $4; exit}')

 #Save to file
 {
  echo "Hash: $HASH"
  echo "Height: $HEIGHT"
  echo "Total: $TOTAL"
  echo "Time: $TIME"
  echo "Relayed By: $RELAYED_BY"
  echo "Prev Block: $PREV_BLOCK"
  echo "-----------------------"
 } >> "$FILE_NAME"


 #move to previous block 
 CURR_HASH=$PREV_BLOCK

done

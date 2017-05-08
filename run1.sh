#!/bin/bash
# echo 'script'
BOT1="$1"
BOT2="./sohamBot/MyBot2"
BOT3="./malina/MyBot"
BOT4="java -jar example_bots/RageBot.jar"
BOT5="java -jar example_bots/BullyBot.jar"
BOT6="java -jar example_bots/DualBot.jar"
BOT7="./zvold/bot-cpp"
BOT8="./final"
# BOT7="./previous/"

if [ "$2" = "soham" ]; then
BOTENEMY="$BOT2"
fi

if [ "$2" = "rage" ]; then
BOTENEMY="$BOT4"
fi

if [ "$2" = "bully" ]; then
BOTENEMY="$BOT5"
fi

if [ "$2" = "dual" ]; then
BOTENEMY="$BOT6"
fi

if [ "$2" = "malina" ]; then
BOTENEMY="$BOT3"
fi

if [ "$2" = "previous" ]; then
BOTENEMY="./previous/MyBot$4"
fi

if [ "$2" = "zvold" ]; then
BOTENEMY="$BOT7"
fi

if [ "$2" = "final" ]; then
BOTENEMY="$BOT8"
fi

	 echo `./playnview maps/map$3.txt 1000 200 log.txt "$BOT1" "$BOTENEMY"` > logsript
#	java -jar tools/PlayGame.jar maps/map$3.txt 1000 200 log.txt "$BOT1" "$BOTENEMY"| java -jar tools/ShowGame.jar
# done

#echo `python countwins.py`
#java -jar tools/PlayGame.jar maps/map5.txt 1000 1000 log.txt "./IBMWatson/MyBot" ".//MyBot"| java -jar tools/ShowGame.jar

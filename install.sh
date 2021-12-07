#!/bin/sh

if [ $(id -u) -eq 0 ]; then
  echo "Instale sem root!"
  exit 1
fi

cp notify $PREFIX/lib/batservice/
if [ $? -ne 0 ]; then
  echo "Precisa compilar o programa!"
  exit 1
fi

cp tools/batservice-notifyc.sh $HOME/.termux/boot/
mv $HOME/.termux/boot/batservice-termux.sh tools/

echo "Terminado!"
echo "by cleds.upper"

if [ $(id -u) -eq 0 ]; then
  echo "Remova sem root!"
  exit 1
fi

rm $HOME/.termux/boot/batservice-notifyc.sh
mv tools/batservice-termux.sh $HOME/.termux/boot/

echo "Terminado!"
echo "by cleds.upper"

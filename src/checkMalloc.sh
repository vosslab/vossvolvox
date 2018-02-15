for j in *.cpp; do echo $j; for i in malloc free; do grep "$i " $j | egrep -v " *\/\/" | wc -l; done; done


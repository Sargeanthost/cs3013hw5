Carlos Medina, Gabriel Curet-Irizarry
cemedina, gocuretirizarry

Keep track of what lines have been summed.
(per thread) Check if your line has been summed. If it has, skip a line and check the if the new line has been summed. else If your previous line is summed, use it and add yourself to it. This will be your sum. If your previous line isnt summed, lock it and s

make array of summed (output) and input vectors. Each thread will be split such that they start at sum 0, then sum 2, then sum 4 etc. This allows for the 

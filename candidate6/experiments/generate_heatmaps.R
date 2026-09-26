#Fut <- read.table("character_valcomp_fut_val.dat")
#pdf("character_valcomp_fut_val.pdf")
#heatmap.2(as.matrix(Fut),dendrogram='none', Rowv=FALSE, Colv=FALSE,trace='none',labRow=c("a","b","c","d","e","g","h","l","m","n","o","p","q","r","s","u","v","w","y","z"),labCol=c("a","b","c","d","e","g","h","l","m","n","o","p","q","r","s","u","v","w","y","z"), col=rev(heat.colors(16)))
library(gplots)

times_heatmap <- function(source) { 
  Table <- read.table(paste(source, "dat", sep="."))
  pdf(paste(source,"pdf",sep="."))
  heatmap.2(as.matrix(log10(Table)),dendrogram='none', Rowv=FALSE, Colv=FALSE,trace='none',labRow=c("a","b","c","d","e","g","h","l","m","n","o","p","q","r","s","u","v","w","y","z"),labCol=c("a","b","c","d","e","g","h","l","m","n","o","p","q","r","s","u","v","w","y","z"), col=heat.colors(16))
  dev.off()
}

value_heatmap <- function(source) { 
  Table <- read.table(paste(source, "dat", sep="."))
  pdf(paste(source,"pdf",sep="."))
  heatmap.2(as.matrix(Table),dendrogram='none', Rowv=FALSE, Colv=FALSE,trace='none',labRow=c("a","b","c","d","e","g","h","l","m","n","o","p","q","r","s","u","v","w","y","z"),labCol=c("a","b","c","d","e","g","h","l","m","n","o","p","q","r","s","u","v","w","y","z"), col=rev(heat.colors(16)))
  dev.off()
}

change_heatmap <- function(source1, source2) { 
  Table1 <- read.table(paste(source1, "dat", sep="."))
  Table2 <- read.table(paste(source2, "dat", sep="."))
  pdf("divided.pdf")
  heatmap.2(as.matrix(Table1/Table2),dendrogram='none', Rowv=FALSE, Colv=FALSE,trace='none',labRow=c("a","b","c","d","e","g","h","l","m","n","o","p","q","r","s","u","v","w","y","z"),labCol=c("a","b","c","d","e","g","h","l","m","n","o","p","q","r","s","u","v","w","y","z"), col=rev(heat.colors(16)))
  dev.off()
}



times_heatmap("characters_valcomp_full_times_lmf")
times_heatmap("characters_valcomp_full_times_binsearch")


times_heatmap("characters_valcomp_full_bbcalls_lmf")
times_heatmap("characters_valcomp_full_bbcalls_binsearch")

value_heatmap("characters_valcomp_full_fut_val")
value_heatmap("characters_valcomp_full_frechet_val")


change_heatmap("characters_valcomp_full_fut_val", "characters_valcomp_full_frechet_val")

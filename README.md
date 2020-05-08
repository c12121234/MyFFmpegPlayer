# MyFFmpegPlayer

## 概述:

基於Qt框架及ffmpeg library做成的播放程式

目前支援軟解 硬解

yuv420p nv12格式

公司的16路監控程式效能消耗蠻大的

cpu使用率妥妥的100%

想寫個硬體加速的功能看能不能改善

在linux環境下 就使用VAAPI

其實若使用Qt原生的QMediaplayer來開發的話

開發速度極快，半天還一天吧

但只開到8路 1080p60 就會卡了...

就先寫個小玩具來練練手，熟悉ffmpeg的使用


## 程式流程:

選定檔案後，進行demux

拆成video和audio

再分別進行decode轉成raw data

audio要resample後再播放

video一樣要將yuv格式的資料進行排序，轉成rgb後才能播放

最後則是要音視訊同步

## 難點 問題:

音視訊同步...

一般來說作法有3種

1. 視訊以音訊為基準 2. 音訊以視訊為基準 3.以額外時鐘為基準

對人體而言，音訊的敏感程度遠大於視訊敏感程度，一點點小斷音就聽得出來

所以2.方案 否決 3.方案複雜 因此以1方案進行


ok,所以這也代表我必須依fps來調整frame的播放

(ex. 每秒60張的速度，一張約16.67ms 等待時間超過它，播放速度會漸慢，反之則變快)

但每個硬體都會有誤差，所以同步也會有誤差

所以"音視訊同步" 這問題

不如說非同步才是常態，只要播放顯示的那個時間段 音視訊有同步就行

大概是這種感覺吧...


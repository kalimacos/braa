
1. OGÓLNIE

  Braa s³u¿y do odpytywania hostów po SNMP - w odró¿nieniu jednak od snmpget
z pakietu net-snmp, braa mo¿e odpytywaæ dziesi±tki czy nawet setki i tysi±ce
hostów jednocze¶nie, i to w dodatku pracuj±c jako jeden proces. Dziêki temu
zu¿ywana jest niewielka ilo¶æ zasobów systemowych a samo odpytywanie (czy w
zasadzie, skanowanie) przebiega BARDZO szybko.

  Braa jest wyposa¿ony w w³asny stos SNMP, nie wymaga wiêc ¿adnych bibliotek
SNMP takich jak net-snmp. Sama implementacja jest bardzo prowizoryczna,
obs³uguje tylko kilka typów danych i z pewno¶ci± nie mo¿e byæ okre¶lona
jako 'zgodna ze standardami'. Zosta³a zaprojektowana tak aby by³a szybka -
i jest szybka. Z tego powodu (i oczywi¶cie z powodu mojego lenistwa), nie
ma analizatora jêzyka ASN.1 w braa - MUSISZ znaæ numeryczne warto¶ci OID'ów
o które chcesz odpytywaæ. (np. .1.3.6.1.2.1.1.5.0, a nie system.sysName.0).

2. WYMAGANIA
  
  * system z rodziny *IX w którym dzia³aj± gniazda BSD i kilka istotnych
    syscalli z POSIX'a
  * du¿a tablica ARP - je¶li chcesz odpytywaæ tysi±ce hostów, potrzebujesz
    systemu który bêdzie w stanie pomie¶ciæ tysi±ce wpisów ARP... na przyk³ad
    zainteresuj siê gc_thresh w linuksie...

  * dobrze jest mieæ te¿ zainstalowany gdzie¶ (niekoniecznie na tej samej
    maszynie) pe³ny pakiet SNMP - braa przyjmuje tylko numeryczne OIDy,
    wiêc czasem mo¿e siê okazaæ, ¿e snmptranslate to ca³kiem przydatne
    narzêdzie...

   Braa nie do koñca jest 'portable' - w ka¿dym razie, zosta³o przetestowane pod
  systemami:
  
   * Linux (shaerrawedd 2.4.19-xfs #7 Fri Oct 4 18:18:38 CEST 2002 i686 unknown)
   * FreeBSD (venom 4.6.2-RELEASE-p10 FreeBSD 4.6.2-RELEASE-p10 #0: Tue Mar 25
           12:59:45 CET 2003     root@venom:/usr/src/sys/compile/VENOM-3  i386)
   * OpenBSD (pantera 3.3 PANTERA#0 i386)
   * SunOS (atlantis 5.9 Generic_112233-04 sun4u sparc SUNW,Ultra-5_10)


3. INSTALACJA

  * przyjrzyj siê Makefile i odkomentuj ustawienia dla Twojego systemu
  * zrób 'make'
  * skopiuj binarkê 'braa' w Twoje ulubione miejsce
   
4. U¯YCIE

  Braa potrzebuje listy zapytañ, okre¶lonych w poni¿szej sk³adni:
  [community@]host[:port]:oid[/id]
  Gdzie: host          to adres IP hosta z którym siê po³±czyæ
         oid           to OID o który spytaæ
		 port          port UDP z którym siê po³±czyæ (domy¶lnie: 161)
         community     to community jakim siê zidentyfikowaæ, domy¶lnie
                       public
		 id            identyfikator zapytania (przydatny przy u¿ywaniu
		               braa w skryptach; sam w sobie nie wp³ywa na proces
					   zapytania)
		 
  Przyk³ady:
     192.168.12.30:.1.3.6.1.2.1.1.5.0
     private@192.168.31.1:.1.3.6.1.2.1.1.6.0
  Mo¿na podaæ tak¿e ca³y zakres hostów, powiedzmy:
     10.253.100.1-10.253.105.254:.1.3.6.1.2.1.2.2.1.10.1
  to to samo co:
     10.253.100.1:.1.3.6.1.2.1.2.2.1.10.1
     10.253.100.2:.1.3.6.1.2.1.2.2.1.10.1
     10.253.100.3:.1.3.6.1.2.1.2.2.1.10.1
     ...
     ...


  Je¶li potrzebujesz odpytaæ hosta (lub zakres hostów) o wiêcej ni¿ jedn±
  warto¶æ SNMP, musisz po prostu wpisaæ kilka osobnych zapytañ:
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.2.1.1.4
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.2.2.1.10.1
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.2.2.1.16.1
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.1.1.6.3
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.2.2.1.3.2
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.4.1.5.3
  Naturalnie braa wykorzystuje mo¿liwo¶æ wys³ania kilku zapytañ SNMP
  w jednym pakiecie i nigdy nie wy¶le wiêcej ni¿ jeden pakiet (poza
  sytuacjami retransmisji je¶li nie ma odpowiedzi) do jednego hosta -
  swoj± drog±, zobacz OGRANICZENIA.

  No, tak jak zosta³o to wspomniane, pierwszy etap to przygotowanie listy
  zapytañ. Listê mo¿na przekazaæ braa na dwa sposoby. Albo zapisaæ j± do pliku
  tekstowego, w którym jedno zapytanie == jedna linijka, albo podaæ seriê
  opcji -q. Mo¿na tak¿e mieszaæ obydwie te metody, za³adowaæ kilka zestawów
  zapytañ z kilku ró¿nych plików, itd.
  
  A oto w³a¶ciwa sk³adnia:
  braa [-a] [-s N] [-r R] [-t T] [-q query [-q query [-q query ...]]] [-f file
       [-f file [-f file ...]]]
       
    WYMAGANE OPCJE:
    -q i -f   to opcje do podawania zapytañ. -q pozwala na dos³owne podanie
              zapytania, -f pozwala na podanie pliku z którego zapytania
              za³adowaæ.
    OPCJE ZMIENIAJ¡CE SZYBKO¦C DZIA£ANIA:
    -s N      okre¶la ile hostów pytaæ w jednej turze, domy¶lnie 50, patrz ni¿ej
    -r R      okre¶la ilo¶æ retransmisji na host, domy¶lnie 3, patrz ni¿ej
    -t T      okre¶la timeout w milisekundach, patrz ni¿ej
    OPCJE ZMIENIAJ¡CE FORMAT WYJ¦CIA
    -a        alternatywny format wyj¶cia

    Wiêc jak to dzia³a?
  
    Tworzona jest kolejka N hostów i wysy³ane s± do niej zapytania. Braa
  czeka do 200 ms na jakie¶ odpowiedzi. Je¶li co¶ przyjdzie, hosty które
  odpowiedzia³y s± usuwane z kolejki i kolejne hosty wchodz± w ich miejsce.
  Ilo¶æ retransmisji dla ka¿dego hosta który nie odpowiedzia³ jest nastêpnie
  zwiêkszana - je¶li ta ilosæ osi±gnie R, host jest równie¿ usuwany
  z kolejki, zwalniaj±c miejsce dla nastêpnego. Host usuniêty z kolejki
  nadal mo¿e wys³aæ odpowied¼ i ZOSTANIE ona przyjêta, st±d opó¼nienia
  pakietów s± bez znaczenia, k³opot jedynie w stratach. Ca³y proces
  jest powtarzany dopóki nie skoñcz± siê hosty do zape³niania nowych
  miejsc w kolejce (tzn. wszyscy zostali ju¿ odpytani lub s± w trakcie
  odpytywania). Kiedy tak nast±pi, kolejka jest stopniowo zmniejszana,
  i kiedy ju¿ osi±gnie d³ugo¶æ 0, braa zamyka wszystkie po³±czenia,
  sortuje wyniki, wy¶wietla je, wychodzi.
    Po T milisekundach od startu, braa pokazuje wyniki i wychodzi
  natychmiast, niezale¿nie od tego ile hostów zosta³o ju¿ odpytanych,
  a do ilu nie posz³o nawet pó³ pakietu...

    Jak widaæ, algorytm nie jest doskona³y. Jak wspomnia³em, chodzi³o o to,
  ¿eby by³ jak najszybszy. Domy¶lne warto¶ci dla -s i -r s± ca³kiem w porz±dku,
  ale oczywi¶cie zawsze mo¿esz je 'podkrêciæ' tak, aby braa chodzi³o jeszcze
  szybciej (ale mniej dok³adnie). Je¶li odpytujesz bardzo du¿± ilo¶æ hostów, nie
  zapomnij zwiêkszyæ znacz±co -t.

    Wyniki s± wypisywane na stdout, jeden-na-liniê:  
    192.168.1.2:.1.3.6.1.2.1.1.5.0:do-wan.elsat.net.pl
    192.168.1.1:.1.3.6.1.2.1.1.4.0:"Mateusz Golicz MG452-RIPE <mtg@elsat.net.pl>"
    192.168.1.1:.1.3.6.1.2.1.1.5.0:ergsun

    Lub je¶li podasz -a, host-na-liniê z wynikami oddzielonymi spacj±, w kolejno¶ci
  w której zosta³y podane zapytania o nie:

    10.253.105.245 358 410 -137 276783656 16315982 4
    10.253.105.244 387 480 -105 6611527 391938 1
    10.253.105.242 371 282 182 0 0 2
    10.253.105.239 373 430 34 0 0 6
    10.253.105.238 354 510 -163 262523537 12319047 4
    
	Przy standardowym trybie wypisywania wyników braa mo¿e przed ka¿dym rezultatem
  zapytania wypisaæ jego podany przez u¿ytkownika identyfikator; ta opcja mo¿e
  byæ przydatna do przetwarzania wyj¶cia z braa w skryptach. Przyk³adowo, zapytanie o:
    192.168.1.2:.1.3.6.1.2.1.1.5.0/Test
    zwróci co¶ w rodzaju:
	Test/192.168.1.2:.1.3.6.1.2.1.1.5.0:do-wan.elsat.net.pl
    
5. PRZYK£ADY

    braa -f queries.lst \
         -q 192.168.1.1:.1.3.6.1.2.1.1.5.0 \
         -q 192.168.1.1:.1.3.6.1.2.1.1.4.0 \
         -q 192.168.1.2:.1.3.6.1.2.1.1.4.0
    
    braa -s 50 -ar 3 -t 250000 -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.2.1.1.4 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.2.2.1.10.1 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.2.2.1.16.1 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.1.1.6.3 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.2.2.1.3.2 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.4.1.5.3

5. B£EDY I OGRANICZENIA

  * jedynymi obs³ugiwanymi typami danych s±: integer (gauge, counter, timeticks,
    etc.), string and OID. Oczywi¶cie zawsze mo¿esz poprawiæ braaasn.c/braaasn.h.
  * braa nigdy nie wy¶le wiêcej ni¿ 1500 bajtów (lub pojedynczy pakiet) do hosta
    w pojedynczej próbie. St±d ilo¶æ zapytañ jakie mo¿esz zadaæ jednemu hostowi
    jest ograniczona. Dodatkowo, kiedy osi±gniesz ten limit podaj±c zbyt
    wiele zapytañ, braa przerwie ca³y proces skanowania. Nie potrafiê okre¶liæ
    ile dok³adnie wynosi ten limit, po prostu zale¿y to od wielu czynnmików
    (g³ównie, d³ugo¶ci OID'ów), w ka¿dym razie, 15 zapytañ na hosta brzmi
    do¶æ niebezpiecznie i lepiej nie przekraczaj tej liczby.
  * nie da siê u¿ywaæ nazw domenowych hostów - zawsze musisz podawaæ IP.
    có¿, to drobiazg, my¶lê ¿e w nastêpnej wersji (o ile bêdzie) zostanie
    to poprawione. 
  * braa prawdopodobnie nie ruszy w ¶rodowiskach 64-bitowych

6. AUTORZY, PODZIEKOWANIA

  Mateusz 'mteg' Golicz <mtg@elsat.net.pl>

7. LICENCJA

  Pakiet rozpowszechniany jest na zasadach licencji GNU GPL, wersja 2.

Mateusz 'mteg' Golicz <mtg@elsat.net.pl>

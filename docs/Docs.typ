= Vykreslovací engine

Vykreslovací engine vytváří 2D obrázek ze vstupních dat o scéně. 2D obrázek si můžeme představit jako 2 rozměrné pole, kde prvku se říká pixel. Ten drží nějakou hodnotu.

Vykreslovací engine je většinou součástí nějakého většího systému, například herního enginu. _Herní engine_ je vícero enginů, například ještě fyzický, pro simulování kolizí objektů, apod. pro interaktivních zážitků.

== Real-time

Vykreslovací engine, který pracuje v reálném čase znamená, že tyto obrázky dokáže vykreslovat takovou rychlostí, že pro lidské oko se jeví jako pohyb. 

== Scéna

Vykreslovací engine potřebuje vstup, který bude říkat, co má vykreslit. Takovým vstupem bude scéna, která bude obsahovat množinu objektů a světel.

Objekt zadefinujeme jako

=== Projekce

Nejprve musíme množinu polygonů zobrazit na dvojrozměrný prostor. To uděláme tzv. projekční maticí.






= Grafická API

= Vulkan

Vulkan je moderní, nízko-úrovňová grafická API. Oproti předchůdcům, jako DirectX 11 nebo OpenGL, dává daleko větší kontrolu nad celým procesem renderování. Mnoho funkcí a optimalizací do té doby dělali samotné ovladače. Vulkan je pouze API, a tím pádem to, jak ve skutečnosti pracuje grafická karta, závisí na ovladači a Vulkan slouží jen jako rozhraní pro požadavky na grafickou kartu.



== Příkazy

Pomocí Vulkan API můžeme grafické kartě posílat požadavky. Tyto příkazy balíme jako balíčky do tzv. _příkazových bufferů_, aneb _command bufferů_. Např. v DirectX 12 se balíčky nazývají _command listy_, což je trochu výstižnější název. Dále jen _příkazové balíčky_ nebo jen _balíčky_, bude-li _příkazové_ zřejmé z kontextu.



=== Recording 

Proces, kdy do balíčku postupně přidáváme příkazy, se nazývá _recording_, neboli _nahrávání_. Nejprve oznámíme začátek nahrávání konkrétního balíčku pomocí funkce _vkBeginCommandBuffer_. Poté přidáváme příkazy pomocí. Nakonec vše ukončíme pomocí `vkEndCommandBuffer`.

Pro představu:

```cpp
vkBeginCommandBuffer(commandBuffer); // začni nahrávat

vkCmdCopyBufferToImage(commandBuffer, ...); // přidej příkaz: zkopíruj obsah nějakého bufferu do obrázku

vkCmdDraw(commandBuffer, ...); // přidej příkaz: vykresli něco

vkEndCommandBuffer(commandBuffer); // ukonči nahrávání

```

Můžeme si všimnout, že volání, které přidá příkaz do balíčku, začíná `vkCmd`.

V předchozích API, jako OpenGL nebo DirectX 11, se příkazy takto neshlukovali. Bylo v režii ovladače zkusit příkazy optimalizovat a poslat do fronty. Nevýhodou bylo, že se muselo vše optimalizovat znovu a znovu každý snímek. Balíčky na druhou stranu můžeme nahrát jednou a posílat je už optimalizované vícekrát. Další výhodou je, že je možné nahrávat více balíčku současně, např. z různých vláken, a tím ještě více program zrychlit. 
// Definovat snímek, ovladač


== Posílání (Submit)

=== Fronta

Když chceme, aby se příkazy z balíčku vykonali, pošleme je do tzv. _fronty_. Protože CPU a GPU nejsou synchronizované, nemůžeme začít výpočet okamžitě. Proto se používá _fronta_, kterou grafická karta postupně vyprazdňuje.

Každá fronta je určité _rodiny_, kde každá _rodina_ umí vykonávat určitou sadu příkazů. Obecně jsou 3 sady:
- Grafická: Především vykreslování, ale jestliže _rodina_ umí tuto sadu, umí i _transfer_ sadu
- Transfer: Pro přesun dat na grafické kartě.
- Compute: 

Fronty ale nejsou tzv. _thread safe_, tedy nejsou připravené, aby se do nich nahrávalo z více vláken na CPU zároveň. Proto se jich vytváří více, většinou jedna pro každé vlákno, které k posílání budeme používat.

== Synchronizace

Je zaručeno, že grafická karta začne balíčky a příkazy v něm vykonávat ve stejném pořadí, jako do fronty přišli. Není ale zaručeno, že skončí ve stejném pořadí. To je problém, protože často se používá vícero iterací, než vznikne výsledný obrázek. Je tedy třeba, aby každá iterace proběhla ve správném pořadí.

=== Command bufferu
Můžeme synchronizovat command buffery pomocí semaforů.

=== Frontu
Příkazy ve frontě můžeme synchronizovat pomocí bariér. 

=== GPU to CPU
Pro synchronizaci mezi grafickou kartou a procesorem je tzv. fence, neboli plot. 


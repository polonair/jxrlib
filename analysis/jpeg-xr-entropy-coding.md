# JPEG XR: энтропийное кодирование DC, LP и HP

Эта заметка описывает реализацию из `image/encode/segenc.c` и
`image/decode/segdec.c`. Это описание фактического порядка операций в
исходном коде, предназначенное как основа managed C#-порта.

## Контекст минимального профиля

Эталонный профиль использует один `Y_ONLY` macroblock 16x16, spatial layout,
QP=1, без overlap и без flexbits. В нём DC, LP и HP последовательно используют
один logical bitstream. Трассировка фиксирует следующие полуоткрытые диапазоны:

| Часть | Диапазон бит | Число бит |
| --- | ---: | ---: |
| DC | 1320-1330 | 10 |
| LP | 1330-1544 | 214 |
| HP | 1544-3502 | 1958 |

Конец HP не обязан быть выровнен по байту: завершающее выравнивание не относится
к данным HP.

## Общая последовательность

После transform, quantization и prediction кодер вызывает:

```text
EncodeMacroblockDC
EncodeMacroblockLowpass
EncodeMacroblockHighpass
```

Декодер выполняет обратный порядок чтения subband-данных:

```text
DecodeMacroblockDC
DecodeMacroblockLowpass
predDCACDec
DecodeMacroblockHighpass
predACDec
```

Prediction не является частью Huffman-кодирования, но определяет значения,
которые кодируются в DC и CBP.

## Общие механизмы

### Bit I/O

`putBit16`, `putBit32`, `getBit16` и `getBit32` записывают и читают поля
заданной длины от старших битов к младшим. Внутренний аккумулятор наполняется
из потока в big-endian порядке. C#-реализация должна воспроизвести это точно:
сдвиг на один бит меняет трактовку всех следующих символов.

### Адаптивные Huffman-таблицы

`CAdaptiveHuffman` хранит текущую таблицу кодов, decode lookup table, один или
два discriminant-счётчика и вклады символов. При записи или чтении символа
счётчики изменяются одинаково с обеих сторон. В точках обновления
`AdaptDiscriminant` выбирает одну из предопределённых таблиц по discriminant.
Таблицы не передаются в bitstream: кодер и декодер обязаны иметь одинаковое
состояние.

Начальный контекст создаётся `ResetCodingContext`: DC model bits равны 8, LP
model bits равны 4, AC model bits равны 0. Для единственного macroblock
минимального fixture последующее изменение моделей уже не влияет на другой
macroblock, но алгоритм обновления необходим для общих изображений.

### Adaptive scan

LP и HP используют изменяемый scan. Каждый элемент `CAdaptiveScan` содержит
`uScan` и счётчик `uTotal`; при ненулевом коэффициенте счётчик растёт. Если он
превышает счётчик предыдущей позиции, элементы меняются местами. Декодер должен
произвести ту же перестановку в том же месте.

Начальная последовательность задаётся `InitZigzagScan`:

- LP: `grgiZigzagInv4x4_lowpass`;
- HP: горизонтальный либо вертикальный scan, зависящий от `iOrientation`.

## DC

`EncodeMacroblockDC` берёт предсказанный квантованный DC из
`iBlockDC[channel][0]`. Для `Y_ONLY`:

```text
m = ModelDC.FlcBits[Y]
a = abs(DC)
q = a >> m

if q != 0:
    write 1
    WriteSignificantAbsLevel(q)
else:
    write 0

write a using m low bits
if a != 0:
    write sign
```

`WriteSignificantAbsLevel` кодирует крупную часть модуля через adaptive Huffman
class и fixed-length extra bits. Для очень больших значений используется
escape: записываются длина дополнительного поля и само значение.

Декодирование зеркально:

```text
q = presence ? ReadSignificantAbsLevel() - 1 : 0
a = (q << m) | read(m)
DC = sign ? -a : a
```

В конце DC вызывается `UpdateModelMB`, обновляющий model bits для будущих
macroblock.

## LP

LP — 15 low-pass коэффициентов каждого канала, позиции 1..15; позиция 0 — DC
и в LP не входит. Каждый коэффициент разделяется на coarse level и residual:

```text
coarse = sign(x) * (abs(x) >> m)
residual = low m bits of abs(x)
```

В минимальном профиле перед первым macroblock `m = 4`.

### LP-CBP

Сначала кодируется наличие хотя бы одного ненулевого coarse coefficient. Для
`Y_ONLY` это один бит. Для YUV применена отдельная адаптивная схема CBP.

### Run/level

`AdaptiveScan` проходит позиции 1..15 и строит список пар:

```text
(numberOfZerosBefore, signedCoarseLevel)
```

Например, последовательность `0, 0, -3, 0, +1` представляется как
`(2, -3), (1, +1)`. Этот список передаётся `EncodeBlock` с context offset
`CTDC`.

### Residual bits

После run/level-последовательности передаются residual для всех 15 позиций.
Для ненулевого coarse level записываются младшие `m` бит модуля; для coarse=0
передаётся значение и, если оно не ноль, sign bit. Декодер сначала восстанавливает
coarse values, затем присоединяет residual и обновляет scan.

## HP

HP состоит из AC-коэффициентов 4x4 blocks macroblock. Для `Y_ONLY` macroblock
это 16 блоков; в каждом обрабатываются позиции 1..15.

### HP-CBP

`predCBPEnc` преобразует текущую маску ненулевых блоков `iCBP` в предсказанную
разность `iDiffCBP`. `CodeCBP` сжимает её двумя уровнями:

1. какие из четырёх групп macroblock содержат активный 4x4 block;
2. точную 4-bit mask активных blocks внутри активной группы.

Для этого используются отдельные adaptive Huffman contexts
`m_pAdaptHuffCBPCY1` и `m_pAdaptHuffCBPCY`. `DecodeCBP` читает разность, после
чего `predCBPDec` восстанавливает исходную `iCBP`.

В эталонном fixture после HP `CBP = 65535`: все шестнадцать grayscale 4x4
blocks содержат coarse AC data.

### Коэффициенты HP

Для каждого блока с установленным CBP:

1. `AdaptiveScan` собирает coarse run/level list;
2. `EncodeBlock` кодирует его с context offset `CTDC + CONTEXTX`;
3. если есть flexbits, младшие биты передаются в отдельный flexbits stream.

В минимальном профиле AC model bits начинаются с нуля, QP=1 и flexbits нет;
поэтому HP run/level хранит точные значения, а отдельного residual stream нет.
Для блока с CBP=0 run/level symbols не записываются.

## Формат `EncodeBlock`

Вход — список `(run, level)` ненулевых коэффициентов. Для первого уровня
формируется контекстный индекс:

```text
SR  = (run == 0)
SL  = (abs(level) > 1)
SRn = 0, если level последний
      1, если следующий level идёт без zero-run
      2, если перед следующим level есть zero-run

index = SRn * 4 + SL * 2 + SR
```

`index` кодируется Huffman-кодом, а sign добавляется младшим битом кодового
слова. Если `SL=0`, модуль равен единице. Если `SL=1`, вызывается
`EncodeSignificantAbsLevel(abs(level)-1)`. Если `SR=0`, длина run передаётся
через `EncodeSignificantRun`.

Для следующих уровней используется более компактный символ, содержащий `SRn`
и `SL`; контекст выбирается также по `iCont` — продолжается ли последовательность
без significant run. `DecodeBlock` выполняет точную обратную операцию.

Важно смещение в `SignificantAbsLevel`: generic block передаёт
`abs(level)-1`, и decoder возвращает уже `abs(level)`. Для DC decoder после
`ReadSignificantAbsLevel` дополнительно вычитает единицу.

## Обновление моделей

После каждого DC/LP/HP этапа `UpdateModelMB` использует число существенных
коэффициентов как Laplacian-like measure и корректирует `m_iFlcBits`. На
границах reset context вызывается `AdaptLowpass*` или `AdaptHighpass*`, которые
применяют `AdaptDiscriminant` к соответствующим Huffman contexts. Граница
контекста определяется tile boundary и группами шириной до 16 macroblocks.

## Порядок переноса decoder на C#

1. Реализовать MSB-first `BitReader`.
2. Перенести статические Huffman tables, lookup tables и `AdaptDiscriminant`.
3. Реализовать DC и `ReadSignificantAbsLevel`.
4. Реализовать LP: CBP, `DecodeBlock`, residual bits и adaptive scan.
5. Реализовать HP: CBP, inverse CBP prediction, `DecodeBlock` и scan.
6. Затем перенести coefficient prediction и inverse transform.

Критические условия совместимости: точная битовая позиция, одинаковая
перестановка adaptive scan, CBP в форме `iDiffCBP` до inverse prediction,
и разные соглашения о единичном смещении DC и generic levels.

# JPEG XR: пайплайн конверсии изображений

Этот документ описывает фактический маршрут данных в исходном проекте `jxrlib`: конвертацию внешнего изображения в `.jxr` и декодирование `.jxr` во внешний формат.

Основные точки входа: `jxrencoderdecoder/JxrEncApp.c`, `jxrencoderdecoder/JxrDecApp.c` и `jxrgluelib/JXRGlueJxr.c`.

## Общая схема

```text
BMP / TIFF / HDR / PNM / YUV
  -> внешний декодер + FormatConverter
  -> пиксели выбранного формата
  -> цветовое преобразование, padding, subsampling
  -> macroblocks 16x16, transform, quantization, prediction
  -> adaptive Huffman, packets, tiles
  -> JPEG XR bitstream
  -> JXR TIFF-подобный контейнер и metadata
  -> .jxr

.jxr
  -> чтение контейнера и image offset
  -> разбор JPEG XR bitstream
  -> entropy decode, prediction, dequantization
  -> inverse transform, overlap, upsampling
  -> обратное цветовое преобразование
  -> BMP / TIFF / HDR / PNM / YUV или буфер пикселей
```

## Конвертация в `.jxr`

1. `JxrEncApp` разбирает аргументы: входной файл, выходной `.jxr`, формат пикселей, качество/квантование, подвыборку цветности, overlap, tiles, alpha и layout потока.

2. По расширению выбирается внешний декодер: BMP, TIFF, HDR, PNM либо raw YUV. Он читает заголовок, размеры, разрешение и пиксели.

3. `PKFormatConverter` приводит исходные данные к запрошенному pixel format: BGR/RGB, grayscale, CMYK, RGBA/BGRA, HDR или YUV.

4. Энкодер получает размеры, DPI, pixel format, внутренний цветовой формат (`Y_ONLY`, `YUV_444`, `YUV_422`, `YUV_420`, `CMYK`), bit depth и режим alpha.

5. Качество преобразуется в quantization parameters. При `quality == 1.0` используются lossless-настройки (QP=1). При меньшем качестве выбирается таблица QP по глубине и внутреннему цветному формату. Если параметры не установлены явно, качество ниже `0.5` выбирает YUV 4:2:0 с двумя уровнями overlap, а от `0.5` — YUV 4:4:4 с одним уровнем overlap.

6. `WriteContainerPre` записывает TIFF-подобный JXR-контейнер: little-endian сигнатуру `II`, WMPhoto ID, PFD/IFD tags, GUID pixel format, размеры, DPI, orientation, offsets и длины image/alpha stream, а также optional ICC, EXIF, GPS, XMP, IPTC, Photoshop и descriptive metadata. Окончательные offsets/lengths обновляются после сжатия.

7. Вход подаётся блоками по 16 строк и делится на macroblocks 16x16. Правый и нижний край дополняются граничными пикселями.

8. `inputMBRow` переводит внешние пиксели во внутренние знаковые целочисленные плоскости. RGB/BGR переводится обратимым lifting-преобразованием в Y/Co/Cg-подобные компоненты; для 4:2:2 и 4:2:0 U/V понижаются фильтром. Для 8-bit данных нулевой уровень смещается относительно 128. Planar alpha обрабатывается как отдельный grayscale stream.

9. Для каждого macroblock выполняются:

   - forward transform: преобразование 4x4 и преобразования уровня macroblock;
   - разложение на DC, LP и HP subbands;
   - quantization;
   - DC/LP/AC и coded-block-pattern prediction;
   - run/level coding и adaptive Huffman coding коэффициентов.

10. Данные упаковываются в tiles и packets. Layout бывает `SPATIAL` (данные macroblock вместе) и `FREQUENCY` (DC, LP, HP, flexbits раздельными packet streams). В bitstream header `WMPHOTO` записываются версия, размеры, tiles, overlap, pixel format, bit depth, alpha, quantizers и index table.

11. Если alpha planar, после основного image stream кодируется отдельный alpha stream. `WriteContainerPost` обновляет в контейнере `ImageOffset`, `ImageByteCount`, `AlphaOffset`, `AlphaByteCount`.

## Конвертация из `.jxr`

1. `JxrDecApp` создаёт JXR decoder через `CreateDecoderFromFile`; этот форк также добавляет `CreateDecoderFromMemory`.

2. `ReadContainer` проверяет `II`, WMPhoto ID и версию, разбирает PFD/IFD и извлекает pixel format, размеры, DPI, orientation, image/alpha offsets и lengths, metadata. Затем поток позиционируется на `ImageOffset`.

3. `ImageStrDecGetInfo` разбирает заголовок `WMPHOTO`: profile/version, размеры, tiles, overlap, subbands, внутренний color format, bit depth, quantizers, alpha и index table. При planar alpha создаётся второе decoder state.

4. Выходной формат выбирается автоматически либо параметром: например grayscale из цветного изображения, CMYK в RGB, BGR/RGB/RGBA/BGRA либо совместимый raw YUV.

5. Для каждого tile/macroblock декодер читает packets. Index table, tiles и ROI позволяют выборочное декодирование. Пропуск LP/HP/flexbits используется для thumbnail или быстрого preview.

6. Для macroblock выполняется обратный алгоритм:

   - Huffman-декодирование DC, LP, HP и flexbits;
   - восстановление coded block patterns и коэффициентов;
   - обратное DC/AC prediction;
   - dequantization;
   - inverse transform;
   - inverse overlap с соседними блоками.

7. При YUV 4:2:0 или 4:2:2 U/V интерполируется до требуемого разрешения. Затем обратное lifting-преобразование восстанавливает RGB/BGR; значения округляются и ограничиваются диапазоном выходного типа.

8. При planar alpha основной и alpha streams декодируются раздельно, после чего alpha записывается в нужный канал целевого пикселя.

9. `PKFormatConverter` передаёт пиксели внешнему encoder для BMP/TIFF/HDR/PNM/YUV. Для `.jxr -> .jxr` предусмотрен compressed-domain transcode с сохранением ROI, orientation, alpha, subbands и layout без обязательного полного преобразования в пиксели.

## Границы будущего managed-порта

Логическое разбиение C#-библиотеки:

- `JxrContainer`: TIFF-подобный контейнер и metadata;
- `JxrBitstream`: headers, bit reader/writer, packets и index table;
- `JxrCodec`: macroblocks, transforms, quantization, prediction и Huffman;
- `PixelConversion`: RGB/BGR/YUV/CMYK/alpha/bit-depth conversion;
- адаптеры BMP/TIFF/HDR/PNM/YUV: независимый внешний слой.

Зависимости декодера: `container -> bitstream header -> bit I/O/Huffman -> coefficients -> inverse transform/overlap -> pixel output`. Для энкодера порядок обратный.

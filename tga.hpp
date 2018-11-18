#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <fstream>

namespace tga
{

#define READPIXEL8(a) \
        red = *a++;

#define READPIXEL24(a) \
        blue = *a++;   \
        green = *a++;  \
        red = *a++;

#define READPIXEL32(a) \
        READPIXEL24(a) \
        alpha = *a++;

#define WRITEPIXEL8(a) \
        *a++ = red;

#define WRITEPIXEL24(a) \
        *a++ = red;     \
        *a++ = green;   \
        *a++ = blue;

#define WRITEPIXEL32(a) \
        WRITEPIXEL24(a) \
		*a++ = alpha;

	struct Header
	{
		uint8_t idlen;
		uint8_t color_map_type;
		uint8_t image_type;

		uint16_t color_map_origin;
		uint16_t color_map_length;
		uint8_t color_map_entry_size;

		uint16_t x_origin;
		uint16_t y_origin;
		uint16_t width;
		uint16_t height;
		uint8_t bits;
		uint8_t image_descriptor;
	};

	enum class ImageFormat
	{
		Monochrome,
		RGB,
		RGBA,
		Undefined
	};

	class TGA
	{
	private:
		uint8_t* Data;
		uint64_t Size;
		uint32_t Width;
		uint32_t Height;
		ImageFormat Format;
	private:
		template <typename Type>
		void RGBPaletted(Type* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size);

		template <typename Type>
		void RGBAPaletted(Type* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size);

		void RGBCompressed(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size);
		void RGBACompressed(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size);
		void MonochromeCompressed(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size);
	public:
		TGA() : Data(nullptr), Size(0), Width(0), Height(0), Format(ImageFormat::Undefined) {}

		uint8_t* GetData() const { return Data; }
		uint64_t GetSize() const { return Size; }
		uint32_t GetWidth() const { return Width; }
		uint32_t GetHeight() const { return Height; }
		ImageFormat GetFormat() const { return Format; }

		bool Load(const std::string& Filename);

		~TGA()
		{
			delete[] Data;
		}
	};

	template <typename Type>
	void TGA::RGBPaletted(Type* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size)
	{
		const int PixelSize = 3;
		int Index;
		int red, green, blue;
		uint8_t* ColorMapPtr;

		for (size_t i = 0; i < Size; i++)
		{
			Index = InBuffer[i];
			ColorMapPtr = &ColorMap[Index * PixelSize];

			READPIXEL24(ColorMapPtr);
			WRITEPIXEL24(OutBuffer);
		}
	}

	template <typename Type>
	void TGA::RGBAPaletted(Type* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size)
	{
		const int PixelSize = 4;
		int Index;
		int red, green, blue, alpha;
		uint8_t* ColorMapPtr;

		for (size_t i = 0; i < Size; i++)
		{
			Index = InBuffer[i];
			ColorMapPtr = &ColorMap[Index * PixelSize];

			READPIXEL32(ColorMapPtr);
			WRITEPIXEL32(OutBuffer);
		}
	}

	void TGA::MonochromeCompressed(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size)
	{
		int header;
		int red;
		size_t pixelcount;

		for (size_t i = 0; i < Size; )
		{
			header = *InBuffer++;
			pixelcount = (header & 0x7F) + 1;

			if (header & 0x80)
			{
				READPIXEL8(InBuffer);
				for (size_t j = 0; j < pixelcount; j++)
				{
					WRITEPIXEL8(OutBuffer);
				}
				i += pixelcount;
			}
			else
			{
				for (size_t j = 0; j < pixelcount; j++)
				{
					READPIXEL8(InBuffer);
					WRITEPIXEL8(OutBuffer);
				}
				i += pixelcount;
			}
		}
	}

	void TGA::RGBCompressed(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size)
	{
		int header;
		int blue, green, red;
		size_t i, j, pixelcount;

		for (i = 0; i < Size; )
		{
			header = *InBuffer++;
			pixelcount = (header & 0x7F) + 1;

			if (header & 0x80)
			{
				READPIXEL24(InBuffer);
				for (j = 0; j < pixelcount; j++)
				{
					WRITEPIXEL24(OutBuffer);
				}
				i += pixelcount;
			}
			else
			{
				for (j = 0; j < pixelcount; j++)
				{
					READPIXEL24(InBuffer);
					WRITEPIXEL24(OutBuffer);
				}
				i += pixelcount;
			}
		}
	}

	void TGA::RGBACompressed(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size)
	{
		int header;
		int blue, green, red, alpha;
		int pix;
		size_t i, j, pixelcount;

		for (i = 0; i < Size; )
		{
			header = *InBuffer++;
			pixelcount = (header & 0x7F) + 1;
			if (header & 0x80)
			{
				READPIXEL32(InBuffer);
				pix = red | (green << 8) | (blue << 16) | (alpha << 24);

				for (j = 0; j < pixelcount; j++)
				{
					memcpy(OutBuffer, &pix, 4);
					OutBuffer += 4;
				}

				i += pixelcount;
			}
			else
			{
				for (j = 0; j < pixelcount; j++)
				{
					READPIXEL32(InBuffer);
					WRITEPIXEL32(OutBuffer);
				}
				i += pixelcount;
			}
		}
	}

	bool TGA::Load(const std::string& Filename)
	{
		std::ifstream File(Filename, std::ios::binary);
		if (!File.is_open()) return false;

		Header Head;
		size_t FileSize = 0;

		File.seekg(0, std::ios_base::end);
		FileSize = File.tellg();
		File.seekg(0, std::ios_base::beg);

		File.read((char*)&Head.idlen,                sizeof(Head.idlen));
		File.read((char*)&Head.color_map_type,       sizeof(Head.color_map_type));
		File.read((char*)&Head.image_type,           sizeof(Head.image_type));
		File.read((char*)&Head.color_map_origin,     sizeof(Head.color_map_origin));
		File.read((char*)&Head.color_map_length,     sizeof(Head.color_map_length));
		File.read((char*)&Head.color_map_entry_size, sizeof(Head.color_map_entry_size));
		File.read((char*)&Head.x_origin,             sizeof(Head.x_origin));
		File.read((char*)&Head.y_origin,             sizeof(Head.y_origin));
		File.read((char*)&Head.width,                sizeof(Head.width));
		File.read((char*)&Head.height,               sizeof(Head.height));
		File.read((char*)&Head.bits,                 sizeof(Head.bits));
		File.read((char*)&Head.image_descriptor,     sizeof(Head.image_descriptor));

		uint8_t* Descriptor = new uint8_t[Head.image_descriptor];
		File.read((char*)Descriptor, Head.image_descriptor);

		size_t ColorMapElementSize = Head.color_map_entry_size / 8;
		size_t ColorMapSize = Head.color_map_length * ColorMapElementSize;
		uint8_t* ColorMap = new uint8_t[ColorMapSize];

		if (Head.color_map_type == 1)
		{
			File.read((char*)ColorMap, ColorMapSize);
		}

		size_t PixelSize = Head.color_map_length == 0 ? (Head.bits / 8) : ColorMapElementSize;
		size_t DataSize = FileSize - sizeof(Header);
		size_t ImageSize = Head.width * Head.height * PixelSize;

		uint8_t* Buffer = new uint8_t[DataSize];
		File.read((char*)Buffer, DataSize);

		Data = new uint8_t[ImageSize];
		memset(Data, 0, ImageSize);

		switch (Head.image_type)
		{
		case 0: break; // No Image
		case 1: // Uncompressed paletted
		{
			if (Head.bits == 8)
			{
				switch (PixelSize)
				{
				case 3: RGBPaletted((uint8_t*)Buffer, ColorMap, Data, Head.width * Head.height);  break;
				case 4: RGBAPaletted((uint8_t*)Buffer, ColorMap, Data, Head.width * Head.height); break;
				}
			}
			else if (Head.bits == 16)
			{
				switch (PixelSize)
				{
				case 3: RGBPaletted((uint16_t*)Buffer, ColorMap, Data, Head.width * Head.height);  break;
				case 4: RGBAPaletted((uint16_t*)Buffer, ColorMap, Data, Head.width * Head.height); break;
				}
			}

			break;
		}
		case 2: // Uncompressed TrueColor
		{
			if (Head.bits = 24 || Head.bits == 32)
			{
				std::copy(&Buffer[0], &Buffer[ImageSize], &Data[0]);

				for (size_t i = 0; i < ImageSize; i += PixelSize)
				{
					std::swap(Data[i], Data[i + 2]);
				}
			}

			break;
		}

		case 3: // Uncompressed Monochrome
		{
			if (Head.bits == 8)
			{
				std::copy(&Buffer[0], &Buffer[ImageSize], &Data[0]);
			}

			break;
		}

		case 9: break; // Compressed paletted TODO
		case 10: // Compressed TrueColor
		{
			if (Head.bits == 24)
			{
				RGBCompressed(Buffer, Data, Head.width * Head.height);
			}
			else if (Head.bits == 32)
			{
				RGBACompressed(Buffer, Data, Head.width * Head.height);
			}

			break;
		}

		case 11: // Compressed Monocrhome
		{
			if (Head.bits == 8)
			{
				MonochromeCompressed(Buffer, Data, Head.width * Head.height);
			}

			break;
		}
		}

		switch (PixelSize)
		{
		case 1: Format = ImageFormat::Monochrome; break;
		case 3: Format = ImageFormat::RGB;        break;
		case 4: Format = ImageFormat::RGBA;       break;
		}

		Width = Head.width;
		Height = Head.height;
		Size = Head.width * Head.height * PixelSize;

		delete[] ColorMap;
		delete[] Descriptor;

		return true;
	}

#undef READPIXEL8
#undef READPIXEL24
#undef READPIXEL32
#undef WRITEPIXEL8
#undef WRITEPIXEL24
#undef WRITEPIXEL32

}



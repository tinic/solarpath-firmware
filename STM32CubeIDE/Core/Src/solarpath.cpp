#include "solarpath.h"

#include "main.h"
#include "app_lorawan.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>

extern "C" ADC_HandleTypeDef hadc;
extern "C" I2C_HandleTypeDef hi2c2;
extern "C" SPI_HandleTypeDef hspi1;
extern "C" SPI_HandleTypeDef hspi2;

class InBitStream {
  public:
	InBitStream(const uint8_t *_buf, size_t _len)
	  : m_buf(_buf),
		m_len(_len),
		m_pos(0),
		m_bitBuf(0),
		m_bitPos(0) {
	}

	void SeekToStart() { m_pos = 0; }
	void SeekTo(size_t pos) { m_pos = pos; }

	void SkipBytes(size_t size) {
		if (m_pos + size < m_len) {
			m_pos += size;
		}
	}

	const uint8_t *Buffer() const { return m_buf; }

	size_t Position() const { return m_pos - (uint32_t(m_bitPos) >> 3); }
	size_t Length() const { return m_len; }

	bool IsEOF() const { return m_pos >= m_len; }

	uint8_t GetUint8() {
		if (m_pos < m_len) {
			return m_buf[m_pos++];
		}
		m_pos++;
		return 0;
	}

	uint16_t GetUint16() {
		return  uint16_t((GetUint8() <<  8)|
						  GetUint8() <<  0);
	}

	uint32_t GetUint32() {
		return  uint32_t((GetUint8() << 24)|
						 (GetUint8() << 16)|
						 (GetUint8() <<  8)|
						  GetUint8() <<  0);
	}

	uint8_t PeekUint8(size_t off) const {
		if (m_pos + off < m_len) {
			return m_buf[m_pos + off];
		}
		return 0;
	}

	void InitBits() {
		m_bitPos = 0;
		m_bitBuf = 0;
	}

	void FlushBits() {
		for (; m_bitPos >= 8; m_bitPos -= 8) {
			m_pos--;
		}
		m_bitBuf = 0;
	}

	uint32_t GetBit() {
		int32_t bPos = m_bitPos;
		uint32_t bBuf = m_bitBuf;
		if (bPos == 0) {
			bBuf = GetUint32();
			bPos = 32;
		}
		uint32_t v = bBuf >> 31;
		bBuf <<= 1;
		bPos -= 1;
		m_bitPos = bPos;
		m_bitBuf = bBuf;
		return v;
	}

	uint32_t GetBits(uint32_t n) {
		if (n > 0 && n <= 32) {
			uint32_t v = 0;
			int32_t bPos = m_bitPos;
			uint32_t bBuf = m_bitBuf;
			if (bPos < int32_t(n)) {
				v   = bBuf >> (32 - bPos);
				n  -= uint32_t(bPos);
				v <<= n;
				bBuf = GetUint32();
				bPos = 32;
			}
			v |= bBuf >> (32 - n);
			if (n != 32)
				bBuf <<= n;
			else
				bBuf = 0;
			bPos -= n;
			m_bitPos = bPos;
			m_bitBuf = bBuf;
			return v;
		} else {
			return 0;
		}
	}

	int32_t GetSBits(uint32_t n) {
		if (n > 0 && n <= 32) {
			uint32_t v = 0;
			int32_t bPos = m_bitPos;
			uint32_t bBuf = m_bitBuf;
			int32_t nSav = int32_t(n);
			if (bPos < int32_t(n)) {
				v   = bBuf >> (32 - bPos);
				n  -= uint32_t(bPos);
				v <<= n;
				bBuf = GetUint32();
				bPos = 32;
			}
			v |= bBuf >> (32-n);
			v |= uint32_t(int32_t((v << (32 - nSav)) &
						 (1L << 31)) >> (32 - nSav));
			if (n != 32)
				bBuf <<= n;
			else
				bBuf = 0;
			bPos  -= n;
			m_bitPos = bPos;
			m_bitBuf = bBuf;
			return int32_t(v);
		} else {
			return 0;
		}
	}

	uint32_t GetExpGolomb() {
		uint32_t b = 0;
		uint32_t v = 0;
		for (; (v = GetBit()) == 0 && b < 32; b++) { }
		v <<= b;
		v |= GetBits(b);
		return v-1;
	}

	int32_t GetSExpGolomb() {
		uint32_t u = GetExpGolomb();
		uint32_t pos = u & 1;
		int32_t s = int32_t((u + 1) >> 1);
		if (pos) {
			return s;
		}
		return -s;
	}

  private:
	const uint8_t *m_buf;
	size_t m_len;
	size_t m_pos;

	uint32_t m_bitBuf;
	int32_t m_bitPos;
};

class OutBitStream {

public:
	OutBitStream()
	  : m_len(sizeof(m_buf)),
		m_pos(0),
		m_bitBuf(0),
		m_bitPos(0) {
	}

	OutBitStream(const OutBitStream &from)
	  : m_len(sizeof(m_buf)),
		m_pos(0),
		m_bitBuf(0),
		m_bitPos(0) {
		PutBytes(from.Buffer(), from.Length());
	}

	~OutBitStream() {
	}

	void Reset() { m_pos = 0; m_len = 0; InitBits(); }

	size_t Position() const { return m_pos; }
	size_t Length() const { return m_len; }
	const uint8_t *Buffer() const { return m_buf; }

	void SetPosition(size_t pos) {
		m_pos = pos;
	}

	void PutUint8(uint8_t v) {
		m_buf[m_pos++] = v;
	}
	void PutUint16(uint16_t v) {
		PutUint8(uint8_t((v>>8)&0xFF));
		PutUint8(uint8_t((v>>0)&0xFF));
	}
	void PutUint32(uint32_t v) {
		PutUint8((v>>24)&0xFF);
		PutUint8((v>>16)&0xFF);
		PutUint8((v>> 8)&0xFF);
		PutUint8((v>> 0)&0xFF);
	}

	void PutBytes(const uint8_t *data, size_t dataLen) {
		for (size_t c = 0; c < dataLen; c++) {
			PutUint8(data[c]);
		}
	}

	void InitBits() {
		m_bitPos = 8;
		m_bitBuf = 0;
	}

	void FlushBits() {
		if (m_bitPos < 8) {
			PutUint8(uint8_t(m_bitBuf));
		}
	}

	void PutBit(uint32_t v) {
		PutBits(uint32_t(v), 1);
	}

	void PutBits(uint32_t v, uint32_t n) {
		if (n <= 0)
			return;
		for (;;) {
			v &= (uint32_t)0xFFFFFFFF >> (32 - n);
			int32_t s = int32_t(n) - m_bitPos;
			if (s <= 0) {
				m_bitBuf |= v << -s;
				m_bitPos -= n;
				return;
			} else {
				m_bitBuf |= v >> s;
				n -= m_bitPos;
				PutUint8(uint8_t(m_bitBuf));
				m_bitBuf = 0;
				m_bitPos = 8;
			}
		}
	}

	void PutSBits(int32_t v, uint32_t n) {
		PutBits(uint32_t(v), n);
	}

	void PutExpGolomb(uint32_t v) {
		v++;
		uint32_t b = 0;
		for (uint32_t g = v; g >>= 1 ; b++) { }
		PutBits(0, b);
		PutBits(v, b+1);
	}

	void PutSExpGolomb(int32_t v) {
		v = 2 * v - 1;
		if (v < 0) {
			v ^= -1;
		}
		PutExpGolomb(uint32_t(v));
	}

	OutBitStream &operator=(const OutBitStream &from) {
		if (&from == this) {
			return *this;
		}
		m_bitBuf = 0;
		m_bitPos = 0;
		Reset();
		PutBytes(from.Buffer(), from.Length());
		return *this;
	}

private:

	uint8_t m_buf[222]; // Max LoraWAN packet size

	size_t m_len;
	size_t m_pos;

	uint32_t m_bitBuf;
	int32_t m_bitPos;
};

class leds {
public:
	static constexpr float auto_threshold_voltage = 0.400f;
	static constexpr size_t num_strips = 3;
	static constexpr size_t num_leds = 3;

	static leds &instance();

	void setrgb(size_t led, uint16_t r, uint16_t g, uint16_t b);

	void setenabled(bool state) { enabled = state; }
	void setauto(bool state) { automode = state; }

	bool on() const { return enabled;  }

	void update() { push(); }

private:

	void init();
	void convert(size_t led, uint16_t r, uint16_t g, uint16_t b);
	void push();

	bool enabled;
	bool automode;
	bool load_enabled;

	static constexpr size_t buf_pad = 8;
	static constexpr size_t buf_size = num_leds*3*16+buf_pad;

	uint16_t red[num_strips];
	uint16_t grn[num_strips];
	uint16_t blu[num_strips];
	uint8_t  buf[num_strips][buf_size];

};

class i2c {
public:
	static i2c &instance();

	void update();

	float temperature() const { return _temperature_1 != 0 ? _temperature_1 :_temperature_0; }
	float humidity() const { return _humidity; }

private:
	float _temperature_0;
	float _temperature_1;
	float _humidity;

	void init();
};


i2c &i2c::instance() {
	static i2c i;
	static bool init = false;
	if (!init) {
		init = true;
		i.init();
	}
	return i;
}

void i2c::init() {
	memset(this, 0, sizeof(i2c));
	update();
}

void i2c::update() {

	const uint8_t at30ts01_address = 0b00110000;
	if (HAL_I2C_IsDeviceReady(&hi2c2, at30ts01_address, 8, HAL_MAX_DELAY) == HAL_OK) {
		uint8_t temp_reg = 0x05;
		uint8_t temp_value[2] = { 0, 0 };
		HAL_I2C_Master_Transmit(&hi2c2, at30ts01_address, (uint8_t *)&temp_reg, sizeof(temp_reg), HAL_MAX_DELAY);
		HAL_I2C_Master_Receive(&hi2c2, at30ts01_address, (uint8_t *)&temp_value, sizeof(temp_value), HAL_MAX_DELAY);
		_temperature_0 = ( ( ( (temp_value[0] << 8) | (temp_value[1] << 0) ) & 0xFFF) ) / 16.0f ;
	}

	const uint8_t en210_address = 0b10000110;
	if (HAL_I2C_IsDeviceReady(&hi2c2, en210_address, 8, HAL_MAX_DELAY) == HAL_OK) {

		uint8_t th_read = 0x30;
		uint8_t th_data[6] = { 0, 0, 0, 0, 0, 0 };
		HAL_I2C_Master_Transmit(&hi2c2, en210_address, (uint8_t *)&th_read, sizeof(th_read), HAL_MAX_DELAY);
		HAL_I2C_Master_Receive(&hi2c2, en210_address, (uint8_t *)&th_data, sizeof(th_data), HAL_MAX_DELAY);

		uint32_t t_val = (th_data[2]<<16) + (th_data[1]<<8) + (th_data[0]<<0);
		uint32_t h_val = (th_data[5]<<16) + (th_data[4]<<8) + (th_data[3]<<0);

		uint32_t t_data = (t_val>>0 ) & 0xffff;
		float TinK = (float)t_data / 64;
		float TinC = TinK - 273.15;
		_temperature_1 = TinC;

		uint8_t th_start = 0x22;
		uint8_t th_select = 0x03;
		HAL_I2C_Master_Transmit(&hi2c2, en210_address, (uint8_t *)&th_start, sizeof(th_start), HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c2, en210_address, (uint8_t *)&th_select, sizeof(th_select), HAL_MAX_DELAY);

		uint32_t h_data = (h_val>>0 ) & 0xffff;
		float H = (float)h_data/51200;
		_humidity = H;
	}
}

class adc {
public:
	static constexpr size_t num_samples = 4;
	static constexpr float system_voltage = 3.4f;
	static constexpr float battery_divider = 2.2f;

	static adc &instance();

	float solar_voltage() const { return _solar; }
	float battery_voltage() const { return _battery; }

	void update();

private:
	uint32_t read_channel(uint32_t channel) const;

	void init();

	float _solar;
	float _battery;

	uint32_t _solar_samples[num_samples];
	size_t _solar_samples_idx;
	uint32_t _battery_samples[num_samples];
	size_t _battery_samples_idx;
};

adc &adc::instance() {
	static adc i;
	static bool init = false;
	if (!init) {
		init = true;
		i.init();
	}
	return i;
}

void adc::init() {
	memset(this, 0, sizeof(adc));
	for (uint32_t c = 0; c < 32; c++) {
		update();
	}
}

uint32_t adc::read_channel(uint32_t channel) const {

    MX_ADC_Init();

    if (HAL_ADCEx_Calibration_Start(&hadc) != HAL_OK) {
    	Error_Handler();
    }

	ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;

    HAL_ADC_ConfigChannel(&hadc, &sConfig);

    HAL_ADC_Start(&hadc);

	HAL_ADC_PollForConversion(&hadc,HAL_MAX_DELAY);

	uint32_t value = HAL_ADC_GetValue(&hadc);
	HAL_ADC_Stop(&hadc);

    HAL_ADC_DeInit(&hadc);

	return value;
}

__attribute__((optimize("Os")))
void adc::update() {
	HAL_GPIO_WritePin(BAT_TEST_GPIO_Port, BAT_TEST_Pin, GPIO_PIN_SET);
	for (volatile uint32_t t = 0; t < 1000; t++) {}
	auto process = [=] (uint32_t channel, uint32_t *samples, size_t &index, float &output) {
		samples[index++] = read_channel(channel);
		index %= num_samples;
		output = 0.0f;
		for (size_t c = 0; c < num_samples; c++) {
			output += float(samples[c]);
		}
		output *= 1.0f / float(num_samples);
		output *= 1.0f / (4095.0f / system_voltage);
	};
	process(ADC_CHANNEL_11, _solar_samples, _solar_samples_idx, _solar);
	HAL_GPIO_WritePin(BAT_TEST_GPIO_Port, BAT_TEST_Pin, GPIO_PIN_RESET);
	process(ADC_CHANNEL_3, _battery_samples, _battery_samples_idx, _battery);
	_battery *= battery_divider;
}

leds &leds::instance() {
	static leds i;
	static bool init = false;
	if (!init) {
		init = true;
		i.init();
	}
	return i;
}

void leds::init() {
	memset(this, 0, sizeof(leds));
}

__attribute__((optimize("Os")))
void leds::convert(size_t led, uint16_t r, uint16_t g, uint16_t b) {
	uint8_t *ptr = &buf[led][0];
	for (uint32_t d = 0; d < buf_pad/2; d++) {
		*ptr++ = 0;
	}
	for (uint32_t d = 0; d < num_leds; d++) {
		auto convert_channel = [=] (uint16_t v, uint8_t *ptr) {
			for (uint32_t c = 0 ; c < 16; c++) {
				if (v&(1UL<<15)) {
					*ptr++ = 0b11110000;
				} else {
					*ptr++ = 0b11000000;
				}
				v<<=1;
			}
		};
		convert_channel(g, ptr); ptr += 16;
		convert_channel(r, ptr); ptr += 16;
		convert_channel(b, ptr); ptr += 16;
	}
	for (uint32_t d = 0; d < buf_pad/2; d++) {
		*ptr++ = 0;
	}
}

__attribute__((optimize("Os")))
void leds::setrgb(size_t led, uint16_t r, uint16_t g, uint16_t b) {
	red[led] = r;
	grn[led] = g;
	blu[led] = b;
	convert(led, r, g, b);
}

__attribute__((optimize("Os")))
void leds::push() {

	auto on = [=] () mutable {
		HAL_GPIO_WritePin(LOAD_ENABLE_GPIO_Port, LOAD_ENABLE_Pin, GPIO_PIN_SET);
		HAL_SPI_Transmit_DMA(&hspi1, &buf[0][0], buf_size);
		HAL_SPI_Transmit_DMA(&hspi2, &buf[1][0], buf_size);
		load_enabled = true;
	};

	auto off = [=] () mutable {
		HAL_GPIO_WritePin(LOAD_ENABLE_GPIO_Port, LOAD_ENABLE_Pin, GPIO_PIN_SET);
		uint8_t ones[16]; memset(ones, 0xFF, sizeof(ones));
		HAL_SPI_Transmit_DMA(&hspi1, &ones[0], sizeof(ones));
		HAL_SPI_Transmit_DMA(&hspi2, &ones[0], sizeof(ones));
		HAL_GPIO_WritePin(LOAD_ENABLE_GPIO_Port, LOAD_ENABLE_Pin, GPIO_PIN_RESET);
		load_enabled = false;
	};

	if (automode && enabled) {
		if (load_enabled) {
			if (adc::instance().solar_voltage() < (auto_threshold_voltage - 0.050)) {
				off();
			} else {
				on();
			}
		} else {
			if (adc::instance().solar_voltage() > (auto_threshold_voltage + 0.050)) {
				on();
			} else {
				off();
			}
		}
	} else {
		if (enabled) {
			on();
		} else {
			off();
		}
	}
}

__attribute__((optimize("Os")))
const uint8_t *encode_packet(uint8_t *len) {
	static OutBitStream bitstream;

	bitstream.Reset();
	uint32_t bt = std::max(int32_t(0), std::min( int32_t(0xF), int32_t(10.0f * (adc::instance().battery_voltage() - 2.7f))));
	uint32_t sv = std::max(int32_t(0), std::min( int32_t(0xF), int32_t(20.0f * (adc::instance().solar_voltage()))));
	uint32_t tp = std::max(int32_t(0), std::min( int32_t(0xFF), int32_t(4.0f * (i2c::instance().temperature() + 10.0f))));
	uint32_t hm = std::max(int32_t(0), std::min( int32_t(0x3F), int32_t(63.0f * (i2c::instance().humidity()))));

	bitstream.PutBits(bt, 4);
	bitstream.PutBits(sv, 4);
	bitstream.PutBits(tp, 8);
	bitstream.PutBits(hm, 6);

	bitstream.PutBits(HAL_GPIO_ReadPin(PGOOD_GPIO_Port, PGOOD_Pin) == GPIO_PIN_SET ? 1 : 0, 1);

	bitstream.PutBits(1, 1);

	bitstream.FlushBits();

	*len = uint8_t(bitstream.Position());
	return bitstream.Buffer();
}

__attribute__((optimize("Os")))
void decode_packet(const uint8_t *packet, uint32_t len) {

	if (len < 7) {
		return;
	}

	InBitStream bitstream(packet, len);

	leds::instance().setenabled(bitstream.GetBits(1));
	leds::instance().setauto(bitstream.GetBits(1));

	auto expand = [=] () mutable {
		uint16_t v = bitstream.GetBits(6);
		v = (v << 10) | (v << 4) | (v >> 2);
		return v;
	};

	leds::instance().setrgb(0,expand(),expand(),expand());
	leds::instance().setrgb(1,expand(),expand(),expand());
	leds::instance().setrgb(2,expand(),expand(),expand());

}

__attribute__((optimize("Os")))
void system_update() {
	adc::instance().update();
	i2c::instance().update();
	leds::instance().update();
}

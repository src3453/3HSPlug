#ifndef EFFECTER_CPP
#include "CMyFilter.cpp"
#include <vector>

#define EFFECTER_CPP

// スルーレート制限付きエンベロープクラス
class SlewLimitedEnvelope
{
private:
  float slewRateUpper; // スルーレート（変化率）
  float slewRateLower; // スルーレート（変化率）
  float currentValue;  // 現在の値
  float targetValue;   // 目標値
  float delta;         // 1サンプルあたりの変化量

public:
  inline SlewLimitedEnvelope(float upper, float lower)
      : slewRateUpper(upper), slewRateLower(lower), currentValue(0.0f), targetValue(0.0f), delta(0.0f) {}

  float process(float target)
  {
    targetValue = target;
    delta = (targetValue - currentValue) * 1.0f; // 1秒間に48000サンプル
    if (currentValue < targetValue)
    {
      currentValue += std::min(delta, slewRateUpper/48000);
    }
    else if (currentValue > targetValue)
    {
      currentValue -= std::min(-delta, slewRateLower/48000);
    }
    if (currentValue < 0.0f)
    {
      currentValue = 0.0f;
    }
    else if (currentValue > 256.0f)
    {
      currentValue = 256.0f;
    }
    return currentValue;
  }

  void changeSlewRate(float upper, float lower)
  {
    slewRateUpper = upper;
    slewRateLower = lower;
  }
};

class S3HS_Effecter
{
private:
  
  /* data */
public:
  CMyFilter lowL, lowR;
  CMyFilter midL, midR;
  CMyFilter highL, highR; // フィルタークラス(https://www.utsbox.com/?page_id=728 より)
  CMyFilter envfilterL, envfilterR;   // 音圧を検知するために使うローパスフィルタ
  SlewLimitedEnvelope gainfilterL; // 急激な音量変化を避けるためのローパスフィルタ
  SlewLimitedEnvelope gainfilterR; // 急激な音量変化を避けるためのローパスフィルタ
  float slewRateUpper = 0.0f; // スルーレート（変化率）
  float slewRateLower = 192000.0f; // スルーレート（変化率）

  S3HS_Effecter() : gainfilterL(slewRateUpper, slewRateLower), gainfilterR(slewRateUpper, slewRateLower) {}

  inline std::vector<std::vector<float>> EQ3band(std::vector<float> inL, std::vector<float> inR,int length, float lowgain, float midgain, float highgain)
  {
    std::vector<float> _out(length, 0);
    std::vector<std::vector<float>> out(2, _out);
    // inL[]、inR[]、outL[]、outR[]はそれぞれ入力信号と出力信号のバッファ(左右)
    // wavelenghtはバッファのサイズ、サンプリング周波数は44100Hzとする

    // エフェクターのパラメーター
    float lowfreq = 400.0f; // 低音域の周波数。50Hz～1kHz程度
    //float lowgain = 2.0f;   // 低音域のゲイン(増幅値)。-15～15dB程度

    float midfreq = 1000.0f; // 中音域の周波数。500Hz～4kHz程度
    //float midgain = -4.0f;   // 中音域のゲイン(増幅値)。-15～15dB程度

    float highfreq = 4000.0f; // 高音域の周波数。1kHz～12kHz程度
    //float highgain = 4.0f;    // 高音域のゲイン(増幅値)。-15～15dB程度

    // 内部変数
    

    // 低音域を持ち上げる(ローシェルフ)フィルタ設定(左右分)
    lowL.LowShelf(lowfreq, 1.0f / sqrt(2.0f), lowgain);
    lowR.LowShelf(lowfreq, 1.0f / sqrt(2.0f), lowgain);
    // 中音域を持ち上げる(ピーキング)フィルタ設定(左右分)
    midL.Peaking(midfreq, 1.0f / sqrt(2.0f), midgain);
    midR.Peaking(midfreq, 1.0f / sqrt(2.0f), midgain);
    // 高音域を持ち上げる(ローシェルフ)フィルタ設定(左右分)
    highL.HighShelf(highfreq, 1.0f / sqrt(2.0f), highgain);
    highR.HighShelf(highfreq, 1.0f / sqrt(2.0f), highgain);

    // 入力信号にエフェクトをかける
    for (int i = 0; i < length; i++)
    {
      // 入力信号にフィルタをかける
      out[0][i] = highL.Process(midL.Process(lowL.Process(inL[i])));
      out[1][i] = highR.Process(midR.Process(lowR.Process(inR[i])));
    }
    return out;
  }

  inline std::vector<std::vector<float>> Compressor(std::vector<float> inL, std::vector<float> inR, int length, float threshold, float ratio, float volume)
  {
    std::vector<float> _out(length, 0);
    std::vector<std::vector<float>> out(2, _out);
    // inL[]、inR[]、outL[]、outR[]はそれぞれ入力信号と出力信号のバッファ(左右)
    // wavelenghtはバッファのサイズ、サンプリング周波数は44100Hzとする

    // エフェクターのパラメーター
    //static float threshold = 0.3; // 圧縮が始まる音圧。0.1～1.0程度
    //static float ratio = 2.0f;    // 圧縮する割合。2.0～10.0程度
    //static float volume = 2.0f;   // 最終的な音量。1.0～3.0程度

    // 内部変数
    // フィルタークラス(https://www.utsbox.com/?page_id=728 より)

    // ローパスフィルターを設定

    // カットオフ周波数が高いほど音圧変化に敏感になる。目安は10～50Hz程度
    envfilterL.LowPass(50.0f, 1.0, 48000.0f);
    envfilterR.LowPass(50.0f, 1.0, 48000.0f);

    // カットオフ周波数が高いほど急激な音量変化になる。目安は5～50Hz程度
    //gainfilterL.LowPass(5.0f, 1.0, 48000.0f);
    //gainfilterR.LowPass(5.0f, 1.0, 48000.0f);
    slewRateUpper = ratio * 48000.0f; // スルーレート（変化率）
    gainfilterL.changeSlewRate(slewRateUpper, slewRateLower);
    gainfilterR.changeSlewRate(slewRateUpper, slewRateLower);
    //printf("Slew rate: %f\n", slewRateLower);

    // 入力信号にエフェクトをかける
    for (int i = 0; i < length; i++)
    {
      // 入力信号の絶対値をとったものをローパスフィルタにかけて音圧を検知する
      float tmpL = envfilterL.Process(abs(inL[i]));
      float tmpR = envfilterR.Process(abs(inR[i]));

      // 音圧をもとに音量(ゲイン)を調整(左)
      float gainL = 1.0f;
      gainL = threshold / tmpL;
      /*if (tmpL > threshold)
      {
        // スレッショルドを超えたので音量(ゲイン)を調節(圧縮)
        gainL = threshold + (tmpL - threshold) / ratio;
      }*/
      // 音量(ゲイン)が急激に変化しないようローパスフィルタを通す
      gainL = gainfilterL.process(gainL);
      //if (!isfinite(gainL))
      //{
      //  gainL = 1.0f;
      //}

      // 左と同様に右も音圧をもとに音量(ゲイン)を調整
      float gainR = 1.0f;
      gainR = threshold / tmpR;
      /*if (tmpR > threshold)
      {
        gainR = threshold + (tmpR - threshold) / ratio;
      }*/
      gainR = gainfilterR.process(gainR);
      //if (!isfinite(gainR))
      //{
      //  gainR = 1.0f;
      //}

      // 入力信号に音量(ゲイン)をかけ、さらに最終的な音量を調整し出力する
      out[0][i] = volume * gainL * inL[i];
      out[1][i] = volume * gainR * inR[i];
    }
    return out;
  }

  /*inline void setSlewRate(float upper, float lower)
  {
    slewRateUpper = upper;
    slewRateLower = lower;
    gainfilterL = SlewLimitedEnvelope(slewRateUpper, slewRateLower);
    gainfilterR = SlewLimitedEnvelope(slewRateUpper, slewRateLower);
  }*/
};
#endif
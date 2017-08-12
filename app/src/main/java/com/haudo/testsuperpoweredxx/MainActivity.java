package com.haudo.testsuperpoweredxx;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import java.io.IOException;

public class MainActivity extends AppCompatActivity {
    boolean playing = false;
    boolean open = false;

    static int REVERB_NON = 0;
    static int REVERB_DRY = 1;
    static int REVERB_WET = 2;
    static int REVERB_WIDTH = 3;
    static int REVERB_MIX = 4;
    static int REVERB_ROOMSIZE = 5;
    static int REVERB_DAMP = 6;

    private TextView txtDry, txtWidth, txtRoomSize, txtMix, txtWet, txtDamp, txtEchoValue;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Get the device's sample rate and buffer size to enable low-latency Android audio output, if available.
        String samplerateString = null, buffersizeString = null;
        if (Build.VERSION.SDK_INT >= 17) {
            AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
            samplerateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
            buffersizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        }
        if (samplerateString == null) samplerateString = "44100";
        if (buffersizeString == null) buffersizeString = "512";

        // Files under res/raw are not zipped, just copied into the APK. Get the offset and length to know where our files are located.
        AssetFileDescriptor fd0 = getResources().openRawResourceFd(R.raw.moc),
                fd1 = getResources().openRawResourceFd(R.raw.beat);
        int fileAoffset = (int) fd0.getStartOffset(), fileAlength = (int) fd0.getLength(), fileBoffset = (int) fd1.getStartOffset(), fileBlength = (int) fd1.getLength();

        try {
            fd0.getParcelFileDescriptor().close();
            fd1.getParcelFileDescriptor().close();
        } catch (IOException e) {
            android.util.Log.d("", "Close error.");
        }


        // Arguments: path to the APK file, offset and length of the two resource files, sample rate, audio buffer size.
        SuperpoweredExample(Integer.parseInt(samplerateString), Integer.parseInt(buffersizeString), getPackageResourcePath(), fileAoffset, fileAlength, fileBoffset, fileBlength);


        // reverb events
        /*final SeekBar fxDry = (SeekBar) findViewById(R.id.reverb_dry);
        fxDry.setOnSeekBarChangeListener(seekBarChangeListener);
        final SeekBar fxWidth = (SeekBar) findViewById(R.id.reverb_width);
        fxWidth.setOnSeekBarChangeListener(seekBarChangeListener);*/


       /* final SeekBar fxWet = (SeekBar) findViewById(R.id.reverb_wet);
        fxWet.setOnSeekBarChangeListener(seekBarChangeListener);
        final SeekBar fxRoomSize = (SeekBar) findViewById(R.id.reverb_roomsize);
        fxRoomSize.setOnSeekBarChangeListener(seekBarChangeListener);
        final SeekBar fxDamp = (SeekBar) findViewById(R.id.reverb_damp);
        fxDamp.setOnSeekBarChangeListener(seekBarChangeListener);*/

        final SeekBar fxMixEcho = (SeekBar) findViewById(R.id.echo_mix);
        fxMixEcho.setOnSeekBarChangeListener(seekBarChangeListener);

        final SeekBar fxMix = (SeekBar) findViewById(R.id.reverb_mix);
        fxMix.setOnSeekBarChangeListener(seekBarChangeListener);

        //txtDry = (TextView) findViewById(R.id.dry_value);
        txtMix = (TextView) findViewById(R.id.reverb_value);
        /*txtRoomSize = (TextView) findViewById(R.id.roomsize_value);
        txtWet = (TextView) findViewById(R.id.wet_value);
        txtWidth = (TextView) findViewById(R.id.width_value);
        txtDamp = (TextView) findViewById(R.id.damp_value);*/
        txtEchoValue = (TextView) findViewById(R.id.echo_value);

    }

    private OnSeekBarChangeListener seekBarChangeListener = new OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            int id = seekBar.getId();
            String value = String.valueOf(progress * 0.01);
            switch (id) {
                /*case R.id.reverb_dry:
                    onFxReverbValue(REVERB_DRY, progress);
                    txtDry.setText(value);
                    break;
                case R.id.reverb_mix:
                    onFxReverbValue(REVERB_MIX, progress);
                    txtMix.setText(value);
                    break;
                case R.id.reverb_roomsize:
                    onFxReverbValue(REVERB_ROOMSIZE, progress);
                    txtRoomSize.setText(value);
                    break;
                case R.id.reverb_wet:
                    onFxReverbValue(REVERB_WET, progress);
                    txtWet.setText(value);
                    break;
                case R.id.reverb_width:
                    onFxReverbValue(REVERB_WIDTH, progress);
                    txtWidth.setText(value);
                    break;
                case R.id.reverb_damp:
                    onFxReverbValue(REVERB_DAMP, progress);
                    txtDamp.setText(value);
                    break;*/
                case R.id.echo_mix:
                    onEcho(progress);
                    txtEchoValue.setText(value);
                    break;

                case R.id.reverb_mix:
               //     onFxReverbValue(REVERB_DRY, progress);
                    onFxReverbValue(REVERB_MIX, progress);
                    onFxReverbValue(REVERB_ROOMSIZE, progress);
              //      onFxReverbValue(REVERB_WET, progress);
                    onFxReverbValue(REVERB_WIDTH, progress);
             //       onFxReverbValue(REVERB_DAMP, progress);

                    txtMix.setText(value);
                    break;
            }
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {

        }
    };


    public void SuperpoweredExample_PlayPause(View button) {  // Play/pause.
        playing = !playing;
        onPlayPause(playing);
        Button b = (Button) findViewById(R.id.playPause);
        if (b != null) b.setText(playing ? "Pause" : "Play");
    }

    private native void SuperpoweredExample(int samplerate, int buffersize, String apkPath, int fileAoffset, int fileAlength, int fileBoffset, int fileBlength);

    private native void onPlayPause(boolean play);

    private native void onFxReverbValue(int reverb_type, int value);

    private native void onEcho(int mix);

    static {
        System.loadLibrary("SuperpoweredExample");
    }
}